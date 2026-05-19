#include <iostream>
#include <string>
#include <cstdint>
#include <algorithm>
#include <filesystem>
#include <vector>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <fstream>
#include <sstream>
#include <cctype>
#include <cmath>

#define WIDTH 900
#define HEIGHT 600

typedef struct ProcEntry {
    std::string name;
    int pid;
    long memUse;
};

// typedef struct GFile {
//     SDL_Texture *icon;
//     SDL_Texture *name;
//     SDL_Texture *size;
//     SDL_Rect icon_pos;
//     SDL_Rect name_pos;
//     SDL_Rect size_pos;
// } GFile;

typedef struct AppData {
    TTF_Font *font;
    std::vector<ProcEntry> processes;
    bool sort_by_memory;
    int scroll_offset;
    Uint32 last_update;

    SDL_Rect sortMemoryButton;
    SDL_Rect sortPidButton;
};

void initialize(AppData *data_ptr);
void handleEvent(SDL_Event *event, AppData *data_ptr);
void render(SDL_Renderer *renderer, AppData *data_ptr);
//void populateDirectory(SDL_Renderer *renderer, AppData *data_ptr);    // unneeded
void listProcesses(std::vector<ProcEntry> &processes, bool sort_by_memory);
//void clearGFiles(std::vector<GFile*>& graphic_entries);       // unneeded
//bool compareFileEntries(const FileEntry *a, const FileEntry *b);      // unneeded
bool pointInRect(int x, int y, SDL_Rect& rect);
void quit(AppData *data_ptr);
void update(AppData *data_ptr);
void drawText(SDL_Renderer *renderer, TTF_Font *font, const std::string& text, int x, int y, SDL_Color color);

int main(int argc, char *argv[])
{
    // Initialize SDL2 (including image and font loaders)
    SDL_Init(SDL_INIT_VIDEO);
    //IMG_Init(IMG_INIT_PNG);       // unneeded, not using images
    TTF_Init();

    // Create window and renderer
    SDL_Window *window = SDL_CreateWindow("Task Manager", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);    // outlines window
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);       // initializes renderer

    // Initialize file browser application
    AppData data;
    //data.current_directory = home;    // unneeded, not dealing with directories
    initialize(&data);

    // Perform render loop
    SDL_Event event;
    event.type = 0;

    do
    {
        update(&data);
        render(renderer, &data);
        if (SDL_WaitEventTimeout(&event, (1000/60)))       // ~60 Hz refresh, instead of waiting for event
        {
            handleEvent(&event, &data);
        }

    } while (event.type != SDL_QUIT);

    // Clean up
    quit(&data);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    //IMG_Quit();       // unneeded, not dealing with images
    SDL_Quit();

    return 0;
}

void initialize(AppData *data_ptr)
{
    // Load font
    data_ptr->font = TTF_OpenFont("resrc/fonts/OpenSans-Regular.ttf", 24);

    data_ptr->sort_by_memory = true;
    data_ptr->scroll_offset = 0;
    data_ptr->last_update = 0;
    data_ptr->sortMemoryButton = {40, 80, 280, 40};     // sort by memory button pos
    data_ptr->sortPidButton = {350, 80, 160, 40};       // sort by PID button pos

    listProcesses(data_ptr->processes, data_ptr->sort_by_memory);     // initial process listing

    // // Load images and create texture
    // SDL_Surface *dir_surf = IMG_Load("resrc/images/directory_icon.png");
    // data_ptr->directory_image = SDL_CreateTextureFromSurface(renderer, dir_surf);
    // SDL_FreeSurface(dir_surf);

    // SDL_Surface *file_surf = IMG_Load("resrc/images/file_icon.png");
    // data_ptr->file_image = SDL_CreateTextureFromSurface(renderer, file_surf);
    // SDL_FreeSurface(file_surf);

    // // Populate with current directory
    // populateDirectory(renderer, data_ptr);
}

void handleEvent(SDL_Event *event, AppData *data_ptr)
{
    // if left click
    if (event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_LEFT)
    {
        int x = event->button.x;    // checks cursor x position
        int y = event->button.y;    // checks sursor y position

        // sort by memory button is clicked
        if (pointInRect(x, y, data_ptr->sortMemoryButton))
        {
            data_ptr->sort_by_memory = true;
            data_ptr->scroll_offset = 0;
            listProcesses(data_ptr->processes, data_ptr->sort_by_memory);
        }

        // sort by PID button is clicked
        else if (pointInRect(x, y, data_ptr->sortPidButton))
        {
            data_ptr->sort_by_memory = false;   // only other sorting mode
            data_ptr->scroll_offset = 0;
            listProcesses(data_ptr->processes, data_ptr->sort_by_memory);
        }

        // for (int i = 0; i < data_ptr->graphic_entries.size(); i++)
        // {
        //     if (pointInRect(x, y, data_ptr->graphic_entries[i]->icon_pos) ||
        //         pointInRect(x, y, data_ptr->graphic_entries[i]->name_pos))
        //     {
        //         // Check if folder or file
        //         //  - folder: make current directory and reset files/graphics
        //         //  - file: open with default app
        //         if (data_ptr->entries[i]->is_directory)
        //         {
        //             data_ptr->current_directory /= data_ptr->entries[i]->name;
        //             clearGFiles(data_ptr->graphic_entries);
        //             populateDirectory(renderer, data_ptr);
        //         }
        //         else
        //         {
        //             pid_t pid = fork();
        //             if (pid == 0)
        //             {
        //                 std::filesystem::path filepath = data_ptr->current_directory / data_ptr->entries[i]->name;
        //                 char* args[3];
        //                 args[0] = const_cast<char*>("/usr/bin/xdg-open");
        //                 args[1] = const_cast<char*>(filepath.c_str());
        //                 args[2] = NULL;
        //                 execvp("/usr/bin/xdg-open", args);
        //             }
        //         }
        //     }
        // }
    }

    else if (event->type == SDL_MOUSEWHEEL)
    {
        data_ptr->scroll_offset -= 25 * event->wheel.y;
        if (data_ptr->scroll_offset < 0)
        {
            data_ptr->scroll_offset = 0;    // can't scroll a "negative" amount
        }

        // for (int i = 0; i < data_ptr->graphic_entries.size(); i++)
        // {
        //     data_ptr->graphic_entries[i]->icon_pos.y += 3 * event->wheel.y;
        //     data_ptr->graphic_entries[i]->name_pos.y += 3 * event->wheel.y;
        //     data_ptr->graphic_entries[i]->size_pos.y += 3 * event->wheel.y;
        // }
    }
}

void render(SDL_Renderer *renderer, AppData *data_ptr)
{
    // this is to make a blue (top) to purple (bottom) gradient background
    for (int y = 0; y < HEIGHT; y++)
    {
        float t = (float)y / HEIGHT;

        Uint8 r = (1 - t) * 0 + t * 255;     // red goes from 0 to 255
        Uint8 b = (1 - t) * 255 + t * 255;   // blue stays at 255

        SDL_SetRenderDrawColor(renderer, r, 0, b, 255);
        SDL_RenderDrawLine(renderer, 0, y, WIDTH, y);
    }

    //SDL_RenderClear(renderer);

    SDL_Color black = {0, 0, 0, 255};
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color gray = {100, 100, 100, 255};
    SDL_Color lightGray = {200, 200, 200, 255};
    SDL_Color blue = {0, 0, 255, 255};
    SDL_Color purple = {128, 0 ,255, 255};
    SDL_Color yellow = {255, 255, 0, 255};
    SDL_Color orange = {255, 128, 0, 255};

    drawText(renderer, data_ptr->font, "Task Manager", 40, 25, yellow);

    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);    // initializes renderer color
    SDL_RenderFillRect(renderer, &data_ptr->sortMemoryButton);      // draws sort by memory button
    drawText(renderer, data_ptr->font, "Sort by Memory Usage", 55, 80, white);      // sort by memory button text

    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);    // initializes renderer color
    SDL_RenderFillRect(renderer, &data_ptr->sortPidButton);      // draws sort by PID button
    drawText(renderer, data_ptr->font, "Sort by PID", 365, 80, white);      // sort by PID button text

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);    // initializes renderer color
    SDL_RenderDrawLine(renderer, 40, 130, 850, 130);    // draws separating line

    drawText(renderer, data_ptr->font, "PID", 50, 145, yellow);      // PID column
    drawText(renderer, data_ptr->font, "Process Name", 150, 145, yellow);    // proc name column
    drawText(renderer, data_ptr->font, "Memory Usage", 500, 145, yellow);    // memory usage column

    // draws a deorative square
    for (int y = 0; y < 80; y++)
    {
        float t = (float)y / 80;

        Uint8 r = (1 - t) * orange.r + t * yellow.r;
        Uint8 g = (1 - t) * orange.g + t * yellow.g;

        SDL_SetRenderDrawColor(renderer, r, g, 0, 255);
        SDL_RenderDrawLine(renderer, 720, 40 + y, 800, 40 + y);
    }

    int Ystart = 180 - data_ptr->scroll_offset;     // y starting position
    int rowHeight = 32;
    long maxMemory = 1;     // proc with highest RAM usage found

    for (const ProcEntry& proc : data_ptr->processes)
    {
        if (proc.memUse > maxMemory) maxMemory = proc.memUse;       // recursively finds next largest memory usage
    }

    // draws every process row
    for (int i = 0; i < data_ptr->processes.size(); i++)
    {
        int y = Ystart + i * rowHeight; // Row vertical position

        if (y < 160 || y > (HEIGHT - 40)) continue;     // skips off-screen rows

        ProcEntry proc = data_ptr->processes[i]; // current process
        SDL_Rect row = {40, y, 860, 28 }; // row rectangle
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);     // initializes renderer color
        SDL_RenderFillRect(renderer, &row);     // draws row background
        drawText(renderer, data_ptr->font, std::to_string(proc.pid), 50, y + 4, black);     // draws PID
        drawText(renderer, data_ptr->font, proc.name, 150, y + 4, black);       // draws proc name
        int barWidth = 0;

        if (maxMemory > 0) barWidth = (int)((proc.memUse / (double)maxMemory) * 250);       // scales memory bar width

        // draws memory bar background
        SDL_Rect memBarBackground = {500, y + 7, 250, 14};
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        SDL_RenderFillRect(renderer, &memBarBackground);

        // draws actual memory bar
        SDL_Rect memBar = {500, y + 7, barWidth, 14};
        SDL_SetRenderDrawColor(renderer, 255, 128, 0, 255);
        SDL_RenderFillRect(renderer, &memBar);

        std::string memText = std::to_string(proc.memUse) + " KB";      // converts memory usage to string
        drawText(renderer, data_ptr->font, memText, 765, y + 4, black);     // draw memory usage text
    }

    // // Draw file entries
    // for (int i = 0; i < data_ptr->graphic_entries.size(); i++)
    // {
    //     SDL_RenderCopy(renderer, data_ptr->graphic_entries[i]->icon, NULL,
    //         &(data_ptr->graphic_entries[i]->icon_pos));

    //     SDL_RenderCopy(renderer, data_ptr->graphic_entries[i]->name, NULL,
    //         &(data_ptr->graphic_entries[i]->name_pos));

    //     SDL_RenderCopy(renderer, data_ptr->graphic_entries[i]->size, NULL,
    //         &(data_ptr->graphic_entries[i]->size_pos));
    // }

    // Draw solid rectangle (teal)
    //rect.x = 440;
    //rect.y = 320;
    //rect.w = 40;
    //rect.h = 30;
    //SDL_SetRenderDrawColor(renderer, 0, 128, 128, 255);
    //SDL_RenderFillRect(renderer, &rect); 

    // Display rendered frame
    SDL_RenderPresent(renderer);
}

// void populateDirectory(SDL_Renderer *renderer, AppData *data_ptr)
// {
//     // Get entries in current directory
//     listFiles(data_ptr->current_directory, data_ptr->entries);

//     // Create texture for text
//     SDL_Color color = { 0, 0, 0 };
//     char file_size_str[32];
//     for (int i = 0; i < data_ptr->entries.size(); i++)
//     {
//         GFile *g_entry = new GFile();

//         const char *name = data_ptr->entries[i]->name.c_str();
//         SDL_Surface *txt_surf = TTF_RenderText_Solid(data_ptr->font, name, color);
//         g_entry->name = SDL_CreateTextureFromSurface(renderer, txt_surf);
//         SDL_FreeSurface(txt_surf);

//         if (data_ptr->entries[i]->is_directory)
//         {
//             g_entry->icon = data_ptr->directory_image;
//             snprintf(file_size_str, 32, "--");
//         }
//         else
//         {
//             g_entry->icon = data_ptr->file_image;
//             snprintf(file_size_str, 32, "%lu bytes", data_ptr->entries[i]->size);
//         }
//         SDL_Surface *size_surf = TTF_RenderText_Solid(data_ptr->font, file_size_str, color);
//         g_entry->size = SDL_CreateTextureFromSurface(renderer, size_surf);
//         SDL_FreeSurface(size_surf);

//         g_entry->icon_pos.w = 28;
//         g_entry->icon_pos.h = 28;
//         g_entry->icon_pos.x = 10;
//         g_entry->icon_pos.y = 50 + (30 * i);

//         SDL_QueryTexture(g_entry->name, NULL, NULL, &(g_entry->name_pos.w), &(g_entry->name_pos.h));
//         g_entry->name_pos.x = 40;
//         g_entry->name_pos.y = 50 + (30 * i);

//         SDL_QueryTexture(g_entry->size, NULL, NULL, &(g_entry->size_pos.w), &(g_entry->size_pos.h));
//         g_entry->size_pos.x = 400;
//         g_entry->size_pos.y = 50 + (30 * i);

//         data_ptr->graphic_entries.push_back(g_entry);
//     }
// }

void listProcesses(std::vector<ProcEntry> &processes, bool sort_by_memory)
{
    // Clear out old data
    processes.clear();

    // Loops through all folders in /proc
    for (const auto& entry : std::filesystem::directory_iterator("/proc"))
    {
        if (!entry.is_directory()) continue;    // skips non-directories
    //     const std::filesystem::directory_entry& entry = *it;     // again, not listing specific directories
        std::string folder_name = entry.path().filename().string();
        bool ispidFolder = true;

        // iterates through each character in folder name to see if they;re all digits
        for (char c : folder_name)
        {
            if (!std::isdigit(c))
            {
                ispidFolder = false;     // not a PID folder if name contains non-digits
                break;
            }
        }

        if (!ispidFolder) continue;      // skips non-process folders

        int pid = std::stoi(folder_name);       // converts folder name into integer PID
        std::ifstream status_file(entry.path() / "status");     // opens /proc/PID/status file

        if (!status_file) continue;     // skips non-status files

        ProcEntry proc;  // default process object
        proc.pid = pid;     // default pid
        proc.name = "[]";   // name placeholder
        proc.memUse = 0;    // memory usage in KB

        std::string line;

        // reads file line by line
        while (std::getline(status_file, line))
        {
            // finds process names
            if (line.rfind("Name:", 0) == 0)
            {
                proc.name = line.substr(6);     // captures idx 6 onwards to account for "Name:\t"
            }

            // finds RAM usage for each process
            else if (line.rfind("VmRSS:", 0) == 0)
            {
                std::stringstream ss(line.substr(6));
                ss >> proc.memUse;
            }
        }

        processes.push_back(proc);      // adds process to be displayed

    //     if (entry_name[0] != '.')
    //     {
    //         FileEntry *file_entry = new FileEntry();
    //         file_entry->name = entry_name;
    //         file_entry->is_directory = entry.is_directory();
    //         if (file_entry->is_directory)
    //         {
    //             file_entry->size = 0;
    //         }
    //         else
    //         {
    //             file_entry->size = entry.file_size();
    //         }
    //         entries.push_back(file_entry);
    //     }
    // }
    }

    if (sort_by_memory)
    {
        std::sort(processes.begin(), processes.end(),
            [](const ProcEntry& a, const ProcEntry& b)
            {
                return a.memUse > b.memUse;
            });
    }
    
    else
    {
        std::sort(processes.begin(), processes.end(),
            [](const ProcEntry& a, const ProcEntry& b)
            {
                return a.pid < b.pid;
            });
    }
}

// bool compareFileEntries(const FileEntry *a, const FileEntry *b)
// {
//     return a->name < b->name;
// }

// void clearGFiles(std::vector<GFile*>& graphic_entries)
// {
//     for (int i = 0; i < graphic_entries.size(); i++)
//     {
//         SDL_DestroyTexture(graphic_entries[i]->name);
//         SDL_DestroyTexture(graphic_entries[i]->size);
//     }
//     graphic_entries.clear();


bool pointInRect(int x, int y, SDL_Rect& rect)
{
    if (x > rect.x && x < rect.x + rect.w &&
        y > rect.y && y < rect.y + rect.h)
    {
        return true;
    }
    return false;
}

void quit(AppData *data_ptr)
{
    // clearGFiles(data_ptr->graphic_entries);
    // SDL_DestroyTexture(data_ptr->directory_image);
    // SDL_DestroyTexture(data_ptr->file_image);
    TTF_CloseFont(data_ptr->font);      // no files or textures needed for this
}

void update(AppData *data_ptr)
{
    Uint32 current_time = SDL_GetTicks();
    if (current_time - data_ptr->last_update >= 500)   // if >= 500 ticks since last update, update
    {
        listProcesses(data_ptr->processes, data_ptr->sort_by_memory);     // relist (refresh) processes
        data_ptr->last_update = current_time;      // update last update time to now
    }
}

void drawText(SDL_Renderer *renderer, TTF_Font *font, const std::string& text, int x, int y, SDL_Color color)
{
    SDL_Surface *surface = TTF_RenderText_Solid(font, text.c_str(), color);     // creates surface with rendered text
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);     // converts the surface we just made into a renderable texture

    SDL_Rect dest;

    dest.x = x;
    dest.y = y;
    dest.w = surface->w;    // sets width to text size
    dest.h = surface->h;    // sets height to text size

    SDL_FreeSurface(surface);       // clears surface, takes load off
    SDL_RenderCopy(renderer, texture, nullptr, &dest);      // draws texture to screen
    SDL_DestroyTexture(texture);    // texture is drawn, destory that bish
}
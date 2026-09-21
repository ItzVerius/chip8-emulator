#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
#else
    #include <stdio.h>
    #include <unistd.h>
    #include <termios.h>
#endif

#include <SDL3/SDL.h>
#include <SDL3/SDL_dialog.h>
#include <stdbool.h>
#include <dirent.h>
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_timer.h"
#include "chip8.h"


#define WINDOW_WIDTH   1280
#define WINDOW_HEIGHT  640
#define FPS 60

#define KEY_UP 1000
#define KEY_DOWN 1001
#define KEY_ENTER 1002
#define KEY_UNKNOWN 1003

static const SDL_Keycode KEYMAP[16] = {
    SDLK_X, // 0            1   2   3   C
    SDLK_1, // 1            4   5   6   D
    SDLK_2, // 2            7   8   9   E
    SDLK_3, // 3            A   0   B   F
    SDLK_Q, // 4
    SDLK_W, // 5               goes to
    SDLK_E, // 6
    SDLK_A, // 7            1   2   3   4
    SDLK_S, // 8            q   w   e   r
    SDLK_D, // 9            a   s   d   f
    SDLK_Z, // A            z   x   c   v
    SDLK_C, // B
    SDLK_4, // C
    SDLK_R, // D            quite confusing
    SDLK_F, // E
    SDLK_V  // F
};

int get_keypress(void) {
#ifdef _WIN32
    int ch = _getch();
    if (ch == 0 || ch == 224) {
        ch = _getch();
        if (ch == 72) return KEY_UP;
        if (ch == 80) return KEY_DOWN;
        return KEY_UNKNOWN;
    }
    if (ch == 13) return KEY_ENTER;
    return ch;
#else
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    ch = getchar();
    if (ch == 27) {
        if (getchar() == '[') {
            ch = getchar();
            if (ch == 'A') ch = KEY_UP;
            else if (ch == 'B') ch = KEY_DOWN;
            else ch = KEY_UNKNOWN;
        }
    } else if (ch == '\n') {
        ch = KEY_ENTER;
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
#endif
}
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    // Initialize SDL3
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        printf("Error initializing SDL: %s\n", SDL_GetError());
        printf("Press ENTER to exit.\n");
        getchar();
        return 1;
    }

    // Chip configuration (initialize, select rom, etc...)
    SDL_Event event;
    bool running = true;
    Chip8 *chip = chip8_init();
    uint32_t pixel_buffer[CHIP8_PIXELCOUNT] = {0};

    printf("Reading roms from directory, select which one you want to run by number:\n");

    char *rom_folder = "./roms";

    DIR *roms_dir = opendir(rom_folder);
    if (!roms_dir){
        perror("Couldn't open roms directory.");
        running = false;
    } else {

        struct dirent *entry;
        size_t index = 1;
        char **rom_list = NULL;
        int rom_count = 0;

        while((entry = readdir(roms_dir)) != NULL){
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            const char *ext = strrchr(entry->d_name, '.');
            if (ext && strcmp(ext, ".ch8") == 0) {
                printf("\t%d) %s\n", (int)index++, entry->d_name);
                rom_list = realloc(rom_list, sizeof(char*) * (rom_count + 1));
                rom_list[rom_count] = strdup(entry->d_name);
                rom_count++;
            }
        }

        closedir(roms_dir);

        if(rom_count == 0){
            fprintf(stderr, "Couldnt find .ch8 files in: %s\n", rom_folder);
            running = false;
        } else {
            #ifdef _WIN32
                HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
                DWORD dwMode = 0;
                GetConsoleMode(hOut, &dwMode);
                dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(hOut, dwMode);
            #endif

            int current_selection = 0;
            bool rom_selected = false;

            while (!rom_selected && running) {
                // \033[H\033[J clears screen and returns to beginning of terminal
                printf("\033[H\033[J");
                printf("=========================================\n");
                printf("  CHIP-8 EMULATOR - SELECT A ROM BELOW   \n");
                printf("     (Use arrows UP/DOWN and ENTER)      \n");
                printf("=========================================\n\n");

                for (int i = 0; i < rom_count; i++) {
                    if (i == current_selection) {
                        printf("\033[7m -> %s \033[0m\n", rom_list[i]); // Inverted background
                    } else {
                        printf("    %s \n", rom_list[i]);
                    }
                }

                int key = get_keypress();

                if (key == KEY_UP && current_selection > 0) {
                    current_selection--;
                } else if (key == KEY_DOWN && current_selection < rom_count - 1) {
                    current_selection++;
                } else if (key == KEY_ENTER) {
                    rom_selected = true;
                } else if (key == 27 || key == 'q' || key == 'Q') {
                    running = false; // Escape or Q aborts
                }
            }

            if (running) {
                char full_path[512];
                snprintf(full_path, sizeof(full_path), "%s/%s", rom_folder, rom_list[current_selection]);

                FILE *rom = fopen(full_path, "rb");
                if (!rom) {
                    perror("Couldn't open ROM file.");
                    running = false;
                } else {
                    fseek(rom, 0, SEEK_END);
                    long rom_size = ftell(rom);
                    rewind(rom);

                    if (rom_size <= 0 || (size_t)rom_size > CHIP8_MAX_PROGRAM_SIZE) {
                        fprintf(stderr, "Invalid ROM size (%ld bytes).\n", rom_size);
                        running = false;
                    } else {
                        uint8_t buffer[CHIP8_MAX_PROGRAM_SIZE];
                        fread(buffer, 1, rom_size, rom);
                        chip8_load(chip, buffer, (size_t)rom_size);
                    }
                    fclose(rom);
                }
            }
        }

        for(int i = 0; i < rom_count; i++){
            free(rom_list[i]);
        }
        free(rom_list);
    }


    // Early return if something went wrong
    if (!running) {
        chip8_destroy(chip);
        SDL_Quit();
        printf("\nPress ENTER to exit...");
        getchar();
        return 1;
    }

    SDL_Window *window = NULL;
        SDL_Renderer *renderer = NULL;
        if (!SDL_CreateWindowAndRenderer("Chip-8 Emulator", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer)) {
            fprintf(stderr, "Error creating window/renderer: %s\n", SDL_GetError());
            SDL_Quit();
            return 1;
        }

        SDL_RaiseWindow(window);



    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                                SDL_TEXTUREACCESS_STREAMING,
                                                CHIP8_WIDTH, CHIP8_HEIGHT);

    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

    // Initialize sound

    SDL_AudioSpec spec;
    spec.channels = 1;
    spec.format = SDL_AUDIO_F32;
    spec.freq = 44100;

    SDL_AudioStream *audio_stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &spec,
        NULL,
        NULL
    );

    if (!audio_stream) {
        fprintf(stderr, "Warning: Couldnt open audio stream: %s\n", SDL_GetError());
    } else {
        SDL_ResumeAudioStreamDevice(audio_stream);
    }

    while (running) {
        Uint64 time = SDL_GetTicks();
        chip8_on_frame_update(chip);

        // Audio checking and playing
        if (audio_stream) {
            if (chip8_get_soundflag(chip)) {
                // Generate samples if stream buffer is becoming empty
                if (SDL_GetAudioStreamQueued(audio_stream) < (int)(sizeof(float) * 735)) {
                    static float wave_phase = 0.0f;
                    float buffer[735];
                    const float frequency = 440.0f;
                    const float volume = 0.2f;

                    for (int i = 0; i < 735; i++) {
                        // Simple square wave
                        buffer[i] = (wave_phase < 0.5f) ? volume : -volume;
                        wave_phase += frequency / 44100.0f;
                        if (wave_phase >= 1.0f) {
                            wave_phase -= 1.0f;
                        }
                    }
                    SDL_PutAudioStreamData(audio_stream, buffer, sizeof(buffer));
                }
            } else {
                // If timer ran out, clear stream
                SDL_ClearAudioStream(audio_stream);
            }
        }

        // Input event handling
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
                bool is_pressed = (event.type == SDL_EVENT_KEY_DOWN);
                for (int i = 0; i < 16; i++) {
                    if (event.key.key == KEYMAP[i]) {
                        chip8_notify_keypad_state(chip, i, is_pressed);
                    }
                }
            }
        }

        // 10 times every frame at 60fps = 600Hz
        for (int i = 0; i < 10; i++) {

            chip8_cycle(chip);

            if(chip8_crashed(chip)){
                running = false;
                break;
            }

            // Only one sprite was rendered each frame in the past
            //if(chip8_get_drawflag(chip)){
            //    break;
            //}
        }

        // Only update if there's some update the GPU needs to render
        if (chip8_get_drawflag(chip)) {
            for (int i = 0; i < CHIP8_PIXELCOUNT; i++) {
                // 0xFFFFFFFF = White, 0x000000FF = Opaque Black
                pixel_buffer[i] = chip8_get_pixelstate(chip, i) ? 0xFFFFFFFF : 0x000000FF;
            }

            SDL_UpdateTexture(texture, NULL, pixel_buffer, CHIP8_WIDTH * sizeof(uint32_t));
            SDL_RenderClear(renderer);
            SDL_RenderTexture(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);

            chip8_end_draw(chip);
        }

        Uint64 frame_time = SDL_GetTicks() - time;
        if (frame_time < (1000 / FPS)) {
            SDL_Delay((1000 / FPS) - frame_time);
        }
    }

    chip8_destroy(chip);

    if (audio_stream) {
        SDL_DestroyAudioStream(audio_stream);
    }
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

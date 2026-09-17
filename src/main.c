#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <dirent.h>
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_timer.h"
#include "chip8.h"

#define WINDOW_WIDTH   1280
#define WINDOW_HEIGHT  640
#define FPS 60

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

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    // Chip configuration (initialize, select rom, etc...)
    uint32_t pixel_buffer[CHIP8_PIXELCOUNT] = {0};

    Chip8 *chip = chip8_init();

    bool running = true;
    SDL_Event event;

    //char roms_route[256];
    //printf("Specify route to your Chip8 programs:\t");
    //scanf(" %s", roms_route);

    char *roms_route = "./roms";
    
    printf("Reading roms from directory, select which one you want to run by number:\n");

    DIR *roms_dir = opendir(roms_route);
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
            fprintf(stderr, "Couldnt find .ch8 files in: %s\n", roms_route);
            running = false;
        } else {
            int option;
            scanf(" %d", &option);
            option -= 1;

            if(option < 0 || option > rom_count - 1){
                running = false;
            } else {
                char full_path[512];
                snprintf(full_path, sizeof(full_path), "%s/%s", roms_route, rom_list[option]);

                FILE *rom = fopen(full_path, "rb");
                if (!rom) {
                    perror("Couldn't open rom file.");
                    running = false;
                } else {
                    // Measure file size
                    fseek(rom, 0, SEEK_END);
                    long rom_size = ftell(rom);
                    rewind(rom);
                
                    const size_t max_size = CHIP8_MAX_PROGRAM_SIZE;
                
                    if (rom_size <= 0 || (size_t)rom_size > max_size) {
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
        return 1;
    }
    
    // Initialize SDL3
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        printf("Error initializing SDL: %s\n", SDL_GetError());
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

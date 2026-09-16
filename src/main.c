#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include "SDL3/SDL_init.h"
#include "chip8.h"

#define WINDOW_WIDTH   640
#define WINDOW_HEIGHT  320
#define FPS 60

static const SDL_Keycode KEYMAP[16] = {
    SDLK_1, SDLK_2, SDLK_3, SDLK_4, // 1, 2, 3, 4
    SDLK_Q, SDLK_W, SDLK_E, SDLK_R, // q, w, e, r
    SDLK_A, SDLK_S, SDLK_D, SDLK_F, // a, s, d, f
    SDLK_Z, SDLK_X, SDLK_C, SDLK_V  // z, x, c, v
};

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    // Initialize SDL3
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        printf("Error inicializando SDL: %s\n", SDL_GetError());
        return 1;
    }
    
    SDL_Window *window = NULL;
        SDL_Renderer *renderer = NULL;
        if (!SDL_CreateWindowAndRenderer("Chip-8 Emulator", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer)) {
            fprintf(stderr, "Error creating window/renderer: %s\n", SDL_GetError());
            SDL_Quit();
            return 1;
        }

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                            SDL_TEXTUREACCESS_STREAMING,
                                            CHIP8_WIDTH, CHIP8_HEIGHT);

    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

    uint32_t pixel_buffer[CHIP8_PIXELCOUNT] = {0};

    Chip8 *chip = chip8_init();

    //TODO Load program into memory

    bool running = true;
    SDL_Event event;

    while (running) {

        chip8_update_timers(chip);
        
        //TODO Audio checking and playing
        
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

        SDL_Delay(16); // ~60 FPS
    }
    
    chip8_destroy(chip);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

#include <SDL.h>
#include <stdio.h>
#include <stdbool.h>

int main(int argc, char* argv[]) {
    // Inicializar SDL2
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("Error inicializando SDL: %s\n", SDL_GetError());
        return 1;
    }

    // Crear una ventana
    SDL_Window* window = SDL_CreateWindow("Chip8 Emulator",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 320, SDL_WINDOW_SHOWN);

    bool running = true;
    SDL_Event event;

    // Bucle infinito hasta que el usuario cierre la ventana
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }
    }

    // Limpiar memoria y salir
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

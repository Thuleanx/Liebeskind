// Disable implicit fallthrough warning
#include "logger/logger.h"
#include "low_level_renderer/graphics_device_interface.h"

#include <cstdlib>

#include <vulkan/vulkan.hpp>
#include <SDL3/SDL.H>
#include <SDL3/SDL_vulkan.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

int main() {
    Logging::initializeLogger();

    GraphicsDeviceInterface GDI;

    while (true) {
        SDL_Event sdl_event;
        bool shouldQuitGame = false;
        while (SDL_PollEvent(&sdl_event)) {
            if (sdl_event.type == SDL_EVENT_QUIT) {
                shouldQuitGame = true;
            }
        }
        if (shouldQuitGame) break;
    }

    return EXIT_SUCCESS;
}

#pragma GCC diagnostic pop

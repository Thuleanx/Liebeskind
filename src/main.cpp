// Disable implicit fallthrough warning
#include "logger/logger.h"
#include "low_level_renderer/graphics_device_interface.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

int main() {
    Logging::initializeLogger();
    GraphicsDeviceInterface GDI =
        GraphicsDeviceInterface::createGraphicsDevice();

    while (true) {
        SDL_Event sdlEvent;
        bool shouldQuitGame = false;

        while (SDL_PollEvent(&sdlEvent)) {
            switch (sdlEvent.type) {
                case SDL_EVENT_QUIT: shouldQuitGame = true; break;
            }
            GDI.handleEvent(sdlEvent);
        }

        if (!GDI.drawFrame()) break;

        if (shouldQuitGame) break;
    }

    return EXIT_SUCCESS;
}

#pragma GCC diagnostic pop

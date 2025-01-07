// Disable implicit fallthrough warning
#include "logger/logger.h"
#include "scene_graph/scene_drawer.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

int main() {
    Logging::initializeLogger();

    SceneDrawer sceneDrawer = SceneDrawer::create();

    while (true) {
        SDL_Event sdlEvent;
        bool shouldQuitGame = false;

        while (SDL_PollEvent(&sdlEvent)) {
            switch (sdlEvent.type) {
                case SDL_EVENT_QUIT: shouldQuitGame = true; break;
            }
            sceneDrawer.handleEvent(sdlEvent);
        }

        if (!sceneDrawer.drawFrame()) break;

        if (shouldQuitGame) break;
    }

    return EXIT_SUCCESS;
}

#pragma GCC diagnostic pop

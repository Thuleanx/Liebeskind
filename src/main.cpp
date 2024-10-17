#include "SDL3/SDL_error.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_video.h"
#include "vulkan/vulkan_core.h"
#include <iostream>
#include <stdexcept>
#include <vector>

#include <SDL3/SDL.H>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#define APP_SHORT_NAME "Game"
#define ENGINE_NAME "Liebeskind"

class SDLException : private std::runtime_error {
public:
    SDLException(const char *message, const int code = 0) : runtime_error(message), code(code) {}

    int get_code() const noexcept { return code; }
private:
    const int code;
};

class SDL {
public:
    explicit SDL(const SDL_InitFlags init_flags) {
        if (!SDL_Init(init_flags)) throw SDLException(SDL_GetError());
    }

    ~SDL() {
        SDL_Quit();
    }

    SDL(const SDL &) = delete;

    const SDL &operator=(const SDL &) = delete;
};

class VulkanLibrary {
public:
    explicit VulkanLibrary(const char *path = nullptr) {
        if (!SDL_Vulkan_LoadLibrary(path)) throw SDLException(SDL_GetError());
    }
};

int main() {
    SDL sdl(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    VulkanLibrary vulkan_library(nullptr);
    SDL_Window* window = SDL_CreateWindow("Liebeskind", 1024, 768, SDL_WINDOW_VULKAN);

    const VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = APP_SHORT_NAME,
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
        .pEngineName = ENGINE_NAME,
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0
    };

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

    SDL_DestroyWindow(window);

    return EXIT_SUCCESS;
}

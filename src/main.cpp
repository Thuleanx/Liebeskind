#include <cstdlib>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <vector>

#include <vulkan/vulkan.hpp>
#include <SDL3/SDL.H>
#include <SDL3/SDL_vulkan.h>

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

    const vk::ApplicationInfo appInfo(
        APP_SHORT_NAME,
        VK_MAKE_VERSION(1, 0, 0),
        ENGINE_NAME,
        VK_MAKE_VERSION(1, 0, 0),
        VK_API_VERSION_1_3
    );

    uint32_t extensions_count;
    const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&extensions_count);

    const vk::InstanceCreateInfo instanceInfo(
        vk::InstanceCreateFlags(),
        &appInfo,
        0,
        nullptr,
        extensions_count,
        extensions
    );

    vk::Instance instance;
    try {
        instance = vk::createInstance(instanceInfo, nullptr);
    } catch (vk::SystemError& err) {
        std::cout << "vk::SystemError: " << err.what() << std::endl;
        return EXIT_FAILURE;
    } catch (std::exception& err) {
        std::cout << "std::exception: " << err.what() << std::endl;
        return EXIT_FAILURE;
    } catch (...) {
        std::cout << "Unknown error" << std::endl;
        return EXIT_FAILURE;
    }

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

    instance.destroy();


    SDL_DestroyWindow(window);

    return EXIT_SUCCESS;
}

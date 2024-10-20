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

std::optional<std::vector<const char*>> getInstanceExtensions() {
    uint32_t count;
    const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&count);

    std::cout << "Query for instance extensions yields:" << std::endl;

    if (!extensions) {
        return {};
    }

    std::vector<const char*> allExtensions;
    for (uint32_t i = 0; i < count; i++) {
        std::cout << i << ". " << extensions[i] << std::endl;
        allExtensions.emplace_back(extensions[i]);
    }

    allExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    return allExtensions;
}

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

    std::optional<std::vector<const char*>> instanceExtensions = getInstanceExtensions();

    if (instanceExtensions == std::nullopt)
        throw SDLException("No supported extensions found");

    const vk::InstanceCreateInfo instanceInfo(
        vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
        &appInfo,
        0,
        nullptr,
        instanceExtensions->size(),
        instanceExtensions->data()
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

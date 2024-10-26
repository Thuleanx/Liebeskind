// Disable implicit fallthrough warning
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <vector>
#include <set>
#include <limits>

#include "plog/Appenders/ColorConsoleAppender.h"
#include "plog/Formatters/TxtFormatter.h"
#include "plog/Severity.h"
#include "plog/Initializers/RollingFileInitializer.h"

#include "plog/Log.h"
#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.hpp>
#include <SDL3/SDL.H>
#include <SDL3/SDL_vulkan.h>

PFN_vkCreateDebugUtilsMessengerEXT  pfnVkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerEXT;

#define APP_SHORT_NAME "Game"
#define ENGINE_NAME "Liebeskind"

#ifndef DEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return this->graphicsFamily.has_value() && this->presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

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


VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT( VkInstance                                 instance,
                                                               const VkDebugUtilsMessengerCreateInfoEXT * pCreateInfo,
                                                               const VkAllocationCallbacks *              pAllocator,
                                                               VkDebugUtilsMessengerEXT *                 pMessenger )
{
  return pfnVkCreateDebugUtilsMessengerEXT( instance, pCreateInfo, pAllocator, pMessenger );
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT( VkInstance instance, VkDebugUtilsMessengerEXT messenger, VkAllocationCallbacks const * pAllocator )
{
  return pfnVkDestroyDebugUtilsMessengerEXT( instance, messenger, pAllocator );
}

std::optional<std::vector<const char*>> getInstanceExtensions() {
    uint32_t count;
    const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&count);

    PLOG_INFO << "Query for instance extensions yields:";

    if (!extensions) {
        return {};
    }

    std::vector<const char*> allExtensions;
    for (uint32_t i = 0; i < count; i++) {
        PLOG_INFO << "\t" << i << ". " << extensions[i];
        allExtensions.emplace_back(extensions[i]);
    }

    PLOG_INFO << "\t" << count << ". " << VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
    allExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    if (enableValidationLayers) {
        PLOG_INFO << "\t" << count << ". " << VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        allExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return allExtensions;
}

bool areValidationLayersSupported() {
    const std::vector<vk::LayerProperties> layerProperties = vk::enumerateInstanceLayerProperties();

    for (const char* layerName : validationLayers) {
        bool layerFound = 0;

        for (const vk::LayerProperties& layerProperty : layerProperties)
            layerFound |= std::strcmp(layerProperty.layerName, layerName) == 0;

        if (!layerFound) 
            return false;
    }

    return true;
}

QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice& device, const vk::SurfaceKHR &surface) {
    QueueFamilyIndices indices;

    std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

    for (uint32_t i = 0; i < queueFamilies.size(); i++) {
        const vk::QueueFamilyProperties& queueFamily = queueFamilies[i];
        if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics))
            indices.graphicsFamily = i;
        if (device.getSurfaceSupportKHR(i, surface))
            indices.presentFamily = i;
    }

    return indices;
}

SwapChainSupportDetails querySwapChainSupport(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface) {
    return {
        .capabilities = device.getSurfaceCapabilitiesKHR(surface),
        .formats = device.getSurfaceFormatsKHR(surface),
        .presentModes = device.getSurfacePresentModesKHR(surface)
    };
}

bool areRequiredDeviceExtensionsSupported(const vk::PhysicalDevice& device) {
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const vk::ExtensionProperties& extension: device.enumerateDeviceExtensionProperties())
        if (requiredExtensions.count(extension.extensionName))
            requiredExtensions.erase(extension.extensionName);

    return requiredExtensions.empty();
}

bool isDeviceSuitable(const vk::PhysicalDevice& device, const vk::SurfaceKHR &surface) {
    vk::PhysicalDeviceProperties deviceProperties = device.getProperties();
    vk::PhysicalDeviceFeatures deviceFeatures = device.getFeatures();

    bool areRequiredExtensionsSupported = areRequiredDeviceExtensionsSupported(device);

    bool isSwapchainAdequate = false;

    if (areRequiredExtensionsSupported) {
        SwapChainSupportDetails swapchainSupport = querySwapChainSupport(device, surface);
        isSwapchainAdequate = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
    }

    return deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu && 
        deviceFeatures.geometryShader && 
        findQueueFamilies(device, surface).isComplete() &&
        areRequiredExtensionsSupported &&
        isSwapchainAdequate;
}

vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    for (const vk::SurfaceFormatKHR& format : availableFormats)
        if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            return format;

    return availableFormats[0];
}

vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
    for (const vk::PresentModeKHR presentMode : availablePresentModes)
        if (presentMode == vk::PresentModeKHR::eMailbox)
            return presentMode;
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, SDL_Window* window) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        // Vulkan framebuffers need the exact pixel size, which 
        // is not often the screen size of the window on high DPI displays
        int width, height;
        SDL_GetWindowSizeInPixels(window, &width, &height);

        vk::Extent2D windowExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height),
        };

        windowExtent.width = std::clamp(windowExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        windowExtent.height = std::clamp(windowExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return windowExtent;
    }
}

bool initializeLogger() {
	// TODO: create log file
    static plog::ColorConsoleAppender<plog::TxtFormatter> appender;
    plog::init(plog::Severity::debug, "liebeskind_runtime.log").addAppender(&appender);
    PLOG_INFO << "Logger initialized.";
	return true;
}

std::optional<vk::PhysicalDevice> getBestPhysicalDevice(const vk::Instance& instance, const vk::SurfaceKHR& surface) {
    std::vector<vk::PhysicalDevice> allPhysicalDevices = instance.enumeratePhysicalDevices();

    for (const vk::PhysicalDevice& device : allPhysicalDevices)
        if (isDeviceSuitable(device, surface))
            return device;
    
    return {};
}

std::optional<vk::Device> createDevice(const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface) {
    QueueFamilyIndices queueFamily = findQueueFamilies(physicalDevice, surface);

    float queuePriority = 1.0f;

    const std::set<uint32_t> uniqueQueueFamilies = {
        queueFamily.graphicsFamily.value(), 
        queueFamily.presentFamily.value()
    };

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    for (uint32_t queueFamilyIndex : uniqueQueueFamilies) {
        vk::DeviceQueueCreateInfo deviceQueueCreateInfo({}, queueFamilyIndex, 1, &queuePriority);
        queueCreateInfos.push_back(deviceQueueCreateInfo);
    }

    const vk::PhysicalDeviceFeatures deviceFeatures;

    const vk::DeviceCreateInfo deviceCreateInfo(
        {},
        queueCreateInfos.size(),
        queueCreateInfos.data(),
        enableValidationLayers ? static_cast<uint32_t>(validationLayers.size()) : 0,
        enableValidationLayers ? validationLayers.data() : nullptr,
        static_cast<uint32_t>(deviceExtensions.size()),
        deviceExtensions.data(),
        &deviceFeatures
    );

    vk::Device device;
    vk::Result result = physicalDevice.createDevice(&deviceCreateInfo, nullptr, &device);

    if (result != vk::Result::eSuccess) {
        PLOG_ERROR << "Failed to create device with result: " << result;
        return {};
    }

    return device;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
    VkDebugUtilsMessageTypeFlagsEXT _messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* _pUserData
) {
    switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            PLOG_VERBOSE << "VulkanValidation: " << pCallbackData->pMessage;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            PLOG_INFO << "VulkanValidation: " << pCallbackData->pMessage;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            PLOG_WARNING << "VulkanValidation: " << pCallbackData->pMessage;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            PLOG_ERROR << "VulkanValidation: " << pCallbackData->pMessage;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
            PLOG_FATAL << "VulkanValidation: " << pCallbackData->pMessage;
    }

    return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, callback, pAllocator);
    }
}

int main() {
    if (!initializeLogger())
        throw SDLException("Logger can't start");

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

    if (enableValidationLayers && !areValidationLayersSupported())
        throw std::runtime_error("Validation layers requested but not available");

    const vk::InstanceCreateInfo instanceInfo(
        vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
        &appInfo,
        enableValidationLayers ? static_cast<uint32_t>(validationLayers.size()) : 0,
        enableValidationLayers ? validationLayers.data() : nullptr,
        instanceExtensions->size(),
        instanceExtensions->data()
    );

    vk::Instance instance;
    vk::DebugUtilsMessengerEXT debugUtilsMessenger;

    try {
        instance = vk::createInstance(instanceInfo, nullptr);

        pfnVkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>( instance.getProcAddr( "vkCreateDebugUtilsMessengerEXT" ) );
        pfnVkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>( instance.getProcAddr( "vkDestroyDebugUtilsMessengerEXT" ) );

        if (enableValidationLayers) {
            vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
            vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
                    vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                    vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                    vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

            debugUtilsMessenger = instance.createDebugUtilsMessengerEXT(
                vk::DebugUtilsMessengerCreateInfoEXT({}, severityFlags, messageTypeFlags, &debugUtilsMessengerCallback));
        }

    } catch (vk::SystemError& err) {
        PLOG_ERROR << "vk::SystemError: " << err.what();
        return EXIT_FAILURE;
    } catch (std::exception& err) {
        PLOG_ERROR << "std::exception: " << err.what();
        return EXIT_FAILURE;
    } catch (...) {
        PLOG_ERROR << "Unknown error";
        return EXIT_FAILURE;
    }

    vk::SurfaceKHR surface;
    {
        VkSurfaceKHR rawSurface;
        if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &rawSurface)) 
            throw new SDLException("Cannot create vulkan surface");
        surface = rawSurface;
    }

    PLOG_INFO << "Surface created";

    const std::optional<vk::PhysicalDevice> suitablePhysicalDevice = getBestPhysicalDevice(instance, surface);

    if (!suitablePhysicalDevice)
        throw new SDLException("No suitable physical devices found");

    const vk::PhysicalDevice physicalDevice = suitablePhysicalDevice.value();
    PLOG_INFO << "Physical device chosen: " << physicalDevice.getProperties().deviceName;

    const std::optional<vk::Device> device_opt = createDevice(physicalDevice, surface);
    if (!device_opt)
        throw new SDLException("Logical device can't be created");

    const vk::Device device = device_opt.value();

    PLOG_INFO << "Logical device created: " << device;

    QueueFamilyIndices queueFamily = findQueueFamilies(physicalDevice, surface);

    vk::Queue graphicsQueue = device.getQueue(queueFamily.graphicsFamily.value(), 0);
    vk::Queue presentQueue = device.getQueue(queueFamily.presentFamily.value(), 0);

    PLOG_INFO << "Available Extensions:";

    for (const auto& extension: vk::enumerateInstanceExtensionProperties())
        PLOG_INFO << "\t" << extension.extensionName;

    // Creating swapchain
    vk::SwapchainKHR swapchain;
    {
        const SwapChainSupportDetails swapchainSupport = querySwapChainSupport(physicalDevice, surface);

        const vk::SurfaceFormatKHR surfaceFormat =  chooseSwapSurfaceFormat(swapchainSupport.formats);
        const vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.presentModes);
        const vk::Extent2D extent = chooseSwapExtent(swapchainSupport.capabilities, window);

        uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
        if (swapchainSupport.capabilities.maxImageCount > 0)
            imageCount = std::min(imageCount, swapchainSupport.capabilities.maxImageCount);
        
        const bool shouldUseExclusiveSharingMode = queueFamily.presentFamily.value() == queueFamily.graphicsFamily.value();
        const uint32_t queueFamilyIndices[] = {queueFamily.presentFamily.value(), queueFamily.graphicsFamily.value()};

        const vk::SharingMode sharingMode = shouldUseExclusiveSharingMode ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent;

        const vk::SwapchainCreateInfoKHR swapchainCreateInfo(
            {},
            surface,
            imageCount,
            surfaceFormat.format,
            surfaceFormat.colorSpace,
            extent,
            1,
            vk::ImageUsageFlagBits::eColorAttachment,
            sharingMode,
            shouldUseExclusiveSharingMode ? 0 : 2,
            shouldUseExclusiveSharingMode ? nullptr : queueFamilyIndices,
            swapchainSupport.capabilities.currentTransform,
            vk::CompositeAlphaFlagBitsKHR::eOpaque,
            presentMode,
            true, // clipped
            nullptr
        );

        swapchain = device.createSwapchainKHR(swapchainCreateInfo, nullptr);
    }

    std::vector<vk::Image> swapchainImages = device.getSwapchainImagesKHR(swapchain);

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

    device.destroySwapchainKHR(std::move(swapchain));
    device.destroy();

    if (enableValidationLayers) instance.destroyDebugUtilsMessengerEXT(std::move(debugUtilsMessenger));
    instance.destroySurfaceKHR(std::move(surface));
    instance.destroy();

    SDL_DestroyWindow(window);

    return EXIT_SUCCESS;
}

#pragma GCC diagnostic pop

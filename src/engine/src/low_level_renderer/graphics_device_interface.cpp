#include <vector>
#include <set>

#include "logger/assert.h"
#include "low_level_renderer/graphics_device_interface.h"

#include "private/swapchain.h"
#include "private/queue_family.h"
#include "private/graphics_device_helper.h"
#include "private/queue_family.h"
#include "private/validation.h"

namespace {
    std::tuple<vk::Instance, vk::DebugUtilsMessengerEXT> init_createInstance() {
        const vk::ApplicationInfo appInfo(
            APP_SHORT_NAME,
            VK_MAKE_VERSION(1, 0, 0),
            ENGINE_NAME,
            VK_MAKE_VERSION(1, 0, 0),
            VK_API_VERSION_1_3
        );

        std::vector<const char*> instanceExtensions = GraphicsHelper::getInstanceExtensions();

        ASSERT(instanceExtensions.size() == 0, "No supported extensions found");
        ASSERT(Validation::shouldEnableValidationLayers && !Validation::areValidationLayersSupported(), "Validation layers enabled but not supported");

        const vk::InstanceCreateInfo instanceInfo(
            vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
            &appInfo,
            Validation::shouldEnableValidationLayers ? static_cast<uint32_t>(Validation::validationLayers.size()) : 0,
            Validation::shouldEnableValidationLayers ? Validation::validationLayers.data() : nullptr,
            instanceExtensions.size(),
            instanceExtensions.data()
        );

        vk::Instance instance = vk::createInstance(instanceInfo, nullptr);
        vk::DebugUtilsMessengerEXT debugUtilsMessenger;

        if (Validation::shouldEnableValidationLayers)
            debugUtilsMessenger = Validation::createDebugMessenger(instance);

        return std::make_tuple(instance, debugUtilsMessenger);
    }


    vk::SurfaceKHR init_createSurface(SDL_Window* window, const vk::Instance& instance) {
        VkSurfaceKHR surface;
        bool isSurfaceCreationSuccessful = SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface);
        ASSERT(isSurfaceCreationSuccessful, "Cannot create surface");
        return surface;
    }

    vk::PhysicalDevice init_createPhysicalDevice(
        const vk::Instance& instance, 
        const vk::SurfaceKHR& surface
    ) {
        std::optional<vk::PhysicalDevice> bestDevice = GraphicsHelper::getBestPhysicalDevice(instance, surface);
        ASSERT(bestDevice.has_value(), "No suitable device found");
        return bestDevice.value_or(vk::PhysicalDevice());
    }

    vk::Device init_createLogicalDevice(
        const vk::PhysicalDevice& physicalDevice, 
        const QueueFamilyIndices& queueFamily
    ) {
        const std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
        
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
            Validation::shouldEnableValidationLayers ? static_cast<uint32_t>(Validation::validationLayers.size()) : 0,
            Validation::shouldEnableValidationLayers ? Validation::validationLayers.data() : nullptr,
            static_cast<uint32_t>(deviceExtensions.size()),
            deviceExtensions.data(),
            &deviceFeatures
        );

        vk::Device device;
        vk::Result result = physicalDevice.createDevice(&deviceCreateInfo, nullptr, &device);

        ASSERT(result == vk::Result::eSuccess, "Failed to create device with result: " << result);

        return device;
    }

    vk::SwapchainKHR init_createSwapchain(
        SDL_Window* window,
        const vk::PhysicalDevice& physicalDevice,
        const vk::Device& device,
        const vk::SurfaceKHR& surface,
        const QueueFamilyIndices& queueFamily
    ) {
        const SwapchainSupportDetails swapchainSupport = Swapchain::querySwapChainSupport(physicalDevice, surface);

        const vk::SurfaceFormatKHR surfaceFormat = Swapchain::chooseSwapSurfaceFormat(swapchainSupport.formats);
        const vk::PresentModeKHR presentMode = Swapchain::chooseSwapPresentMode(swapchainSupport.presentModes);
        const vk::Extent2D extent = Swapchain::chooseSwapExtent(swapchainSupport.capabilities, window);

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

        return device.createSwapchainKHR(swapchainCreateInfo, nullptr);
    }
}

GraphicsDeviceInterface::GraphicsDeviceInterface() {
    isConstructionSuccessful = true;

    const SDL_InitFlags initFlags = SDL_INIT_VIDEO | SDL_INIT_EVENTS;

    const bool isSDLInitSuccessful = SDL_Init(initFlags);
    ASSERT(isSDLInitSuccessful, "SDL cannot be initialized with flag " << initFlags << " error: " << SDL_GetError());
    isConstructionSuccessful &= isSDLInitSuccessful;
    if (!isSDLInitSuccessful) return;


    const bool isVulkanLibraryLoadSuccessful = SDL_Vulkan_LoadLibrary(nullptr);
    assert(isVulkanLibraryLoadSuccessful && "Vulkan library cannot be loaded");
    isConstructionSuccessful &= isVulkanLibraryLoadSuccessful;
    if (!isVulkanLibraryLoadSuccessful) return;

    window = SDL_CreateWindow("Liebeskind", 1024, 768, SDL_WINDOW_VULKAN);

    std::tie(instance, debugUtilsMessenger) = init_createInstance();

    surface = init_createSurface(window, instance);

    physicalDevice = init_createPhysicalDevice(instance, surface);

    QueueFamilyIndices queueFamily = QueueFamilyIndices::findQueueFamilies(physicalDevice, surface);

    device = init_createLogicalDevice(physicalDevice, queueFamily);

    graphicsQueue = device.getQueue(queueFamily.graphicsFamily.value(), 0);
    presentQueue = device.getQueue(queueFamily.presentFamily.value(), 0);

    LLOG_INFO << "Available Extensions:";
    for (const auto& extension: vk::enumerateInstanceExtensionProperties())
        LLOG_INFO << "\t" << extension.extensionName;

    swapchain = init_createSwapchain(
            window, physicalDevice, device, surface, queueFamily);
}

GraphicsDeviceInterface::~GraphicsDeviceInterface() {
    device.destroySwapchainKHR(std::move(swapchain));
    device.destroy();

    if (Validation::shouldEnableValidationLayers)
        instance.destroyDebugUtilsMessengerEXT(debugUtilsMessenger);

    instance.destroySurfaceKHR(surface);
    instance.destroy();

    SDL_DestroyWindow(window);
    SDL_Quit();
}

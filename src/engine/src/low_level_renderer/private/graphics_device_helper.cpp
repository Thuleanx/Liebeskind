#include "graphics_device_helper.h"

#include <SDL3/SDL_vulkan.h>

#include <set>
#include <vulkan/vulkan.hpp>

#include "core/logger/vulkan_ensures.h"
#include "core/logger/logger.h"
#include "swapchain.h"
#include "low_level_renderer/queue_family.h"
#include "validation.h"

const std::vector<const char*> GraphicsHelper::deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

std::vector<const char*> GraphicsHelper::getInstanceExtensions() {
    uint32_t count;
    const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&count);
    LLOG_INFO << "Query for instance extensions yields:";
    std::vector<const char*> allExtensions;

    if (!extensions) return allExtensions;

    for (uint32_t i = 0; i < count; i++) {
        LLOG_INFO << "\t" << i << ". " << extensions[i];
        allExtensions.emplace_back(extensions[i]);
    }

    LLOG_INFO << "\t" << count << ". "
              << VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
    allExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    if (Validation::shouldEnableValidationLayers) {
        LLOG_INFO << "\t" << count << ". " << VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        allExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return allExtensions;
}

std::optional<vk::PhysicalDevice> GraphicsHelper::getBestPhysicalDevice(
    const vk::Instance& instance, const vk::SurfaceKHR& surface
) {
    const vk::ResultValue<std::vector<vk::PhysicalDevice>> allPhysicalDevices =
        instance.enumeratePhysicalDevices();
    VULKAN_ENSURE_SUCCESS(
        allPhysicalDevices.result, "Can't enumerate all physical devices:"
    );
    for (const vk::PhysicalDevice& device : allPhysicalDevices.value)
        if (isDeviceSuitable(device, surface)) return device;
    return {};
}

bool GraphicsHelper::isDeviceSuitable(
    const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface
) {
    vk::PhysicalDeviceProperties deviceProperties =
        physicalDevice.getProperties();
    vk::PhysicalDeviceFeatures deviceFeatures = physicalDevice.getFeatures();
    const bool areRequiredExtensionsSupported =
        areRequiredDeviceExtensionsSupported(physicalDevice);
    bool isSwapchainAdequate = false;
    if (areRequiredExtensionsSupported) {
        isSwapchainAdequate =
            !Swapchain::getSupportedColorFormats(physicalDevice, surface)
                 .empty() &&
            !Swapchain::getSupportedPresentModes(physicalDevice, surface)
                 .empty();
    }
    const bool isAnisotropicFilteringSupported =
        deviceFeatures.samplerAnisotropy == vk::True;

    return deviceProperties.deviceType ==
               vk::PhysicalDeviceType::eDiscreteGpu &&
           deviceFeatures.geometryShader &&
           QueueFamilyIndices::findQueueFamilies(physicalDevice, surface)
               .isComplete() &&
           areRequiredExtensionsSupported && isSwapchainAdequate &&
           isAnisotropicFilteringSupported;
}

bool GraphicsHelper::areRequiredDeviceExtensionsSupported(
    const vk::PhysicalDevice& device
) {
    std::set<std::string> requiredExtensions(
        deviceExtensions.begin(), deviceExtensions.end()
    );
    const vk::ResultValue<std::vector<vk::ExtensionProperties>> extensions =
        device.enumerateDeviceExtensionProperties();
    VULKAN_ENSURE_SUCCESS(
        extensions.result, "Can't enumerate all device extensions:"
    );

    for (const vk::ExtensionProperties& extension : extensions.value)
        if (requiredExtensions.count(extension.extensionName))
            requiredExtensions.erase(extension.extensionName);

    return requiredExtensions.empty();
}

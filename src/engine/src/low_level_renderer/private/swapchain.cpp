#include "swapchain.h"

#include <limits>
#include <optional>

#include "logger/vulkan_ensures.h"
#include "image.h"

std::vector<vk::SurfaceFormatKHR> Swapchain::getSupportedColorFormats(
    const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface
) {
    const vk::ResultValue<std::vector<vk::SurfaceFormatKHR>> surfaceFormatsGet =
        physicalDevice.getSurfaceFormatsKHR(surface);
    VULKAN_ENSURE_SUCCESS(
        surfaceFormatsGet.result, "Can't get surface format:"
    );
    return surfaceFormatsGet.value;
}

std::vector<vk::PresentModeKHR> Swapchain::getSupportedPresentModes(
    const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface
) {
    const vk::ResultValue<std::vector<vk::PresentModeKHR>> presentModeGet =
        physicalDevice.getSurfacePresentModesKHR(surface);
    VULKAN_ENSURE_SUCCESS(presentModeGet.result, "Can't get present mode:");
    return presentModeGet.value;
}

vk::SurfaceCapabilitiesKHR Swapchain::getSurfaceCapability(
    const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface
) {
    const vk::ResultValue<vk::SurfaceCapabilitiesKHR> surfaceCapabilityGet =
        physicalDevice.getSurfaceCapabilitiesKHR(surface);
    VULKAN_ENSURE_SUCCESS(
        surfaceCapabilityGet.result, "Can't get surface capability:"
    );
    return surfaceCapabilityGet.value;
}

vk::SurfaceFormatKHR Swapchain::chooseSwapSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR>& availableFormats
) {
    for (const vk::SurfaceFormatKHR& format : availableFormats)
        if (format.format == vk::Format::eB8G8R8A8Srgb &&
            format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            return format;

    return availableFormats[0];
}

vk::PresentModeKHR Swapchain::chooseSwapPresentMode(
    const std::vector<vk::PresentModeKHR>& availablePresentModes
) {
    for (const vk::PresentModeKHR presentMode : availablePresentModes)
        if (presentMode == vk::PresentModeKHR::eMailbox) return presentMode;

    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Swapchain::chooseSwapExtent(
    const vk::SurfaceCapabilitiesKHR& capabilities, SDL_Window* window
) {
    if (capabilities.currentExtent.width !=
        std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;
    else {
        // Vulkan framebuffers need the exact pixel size, which
        // is not often the screen size of the window on high DPI displays
        int width, height;
        SDL_GetWindowSizeInPixels(window, &width, &height);
        vk::Extent2D windowExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height),
        };
        windowExtent.width = std::clamp(
            windowExtent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width
        );
        windowExtent.height = std::clamp(
            windowExtent.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height
        );
        return windowExtent;
    }
}

vk::SurfaceFormatKHR Swapchain::getSuitableColorAttachmentFormat(
    const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface
) {
    return Swapchain::chooseSwapSurfaceFormat(
        getSupportedColorFormats(physicalDevice, surface)
    );
}

vk::Format Swapchain::getSuitableDepthAttachmentFormat(
    const vk::PhysicalDevice& physicalDevice
) {
    const std::optional<vk::Format> suitableDepthFormat =
        Image::findSupportedFormat(
            physicalDevice,
            {vk::Format::eD32Sfloat,
             vk::Format::eD32SfloatS8Uint,
             vk::Format::eD24UnormS8Uint},
            vk::ImageTiling::eOptimal,
            vk::FormatFeatureFlagBits::eDepthStencilAttachment
        );
    ASSERT(
        suitableDepthFormat.has_value(),
        "Can't find suitable format for depth buffer"
    );
    return suitableDepthFormat.value();
}

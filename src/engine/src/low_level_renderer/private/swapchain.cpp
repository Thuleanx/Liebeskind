#include "swapchain.h"

SwapchainSupportDetails Swapchain::querySwapChainSupport(
    const vk::PhysicalDevice &device, 
    const vk::SurfaceKHR &surface
) {
    return {
        .capabilities = device.getSurfaceCapabilitiesKHR(surface),
        .formats = device.getSurfaceFormatsKHR(surface),
        .presentModes = device.getSurfacePresentModesKHR(surface)
    };
}

vk::SurfaceFormatKHR Swapchain::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    for (const vk::SurfaceFormatKHR& format : availableFormats)
        if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            return format;

    return availableFormats[0];
}

vk::PresentModeKHR Swapchain::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
    for (const vk::PresentModeKHR presentMode : availablePresentModes)
        if (presentMode == vk::PresentModeKHR::eMailbox)
            return presentMode;
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Swapchain::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, SDL_Window* window) {
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
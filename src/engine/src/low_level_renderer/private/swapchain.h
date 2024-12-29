#pragma once

#include <vulkan/vulkan.hpp>
#include <SDL3/SDL_vulkan.h>

namespace Swapchain {
std::vector<vk::SurfaceFormatKHR> getSupportedColorFormats(
    const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface
);

std::vector<vk::PresentModeKHR> getSupportedPresentModes(
    const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface
);

vk::SurfaceCapabilitiesKHR getSurfaceCapability(
    const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface
);

vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR>& availableFormats
);

vk::PresentModeKHR chooseSwapPresentMode(
    const std::vector<vk::PresentModeKHR>& availablePresentModes
);

vk::Extent2D chooseSwapExtent(
    const vk::SurfaceCapabilitiesKHR& capabilities, SDL_Window* window
);

vk::SurfaceFormatKHR getSuitableColorAttachmentFormat(
    const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface
);

vk::Format getSuitableDepthAttachmentFormat(
    const vk::PhysicalDevice& physicalDevice
);
}  // namespace Swapchain

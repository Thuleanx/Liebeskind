#pragma once

#include <optional>
#include <vulkan/vulkan.hpp>

namespace graphics {
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete();

    static QueueFamilyIndices findQueueFamilies(
        const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface
    );
};
}  // namespace Graphics

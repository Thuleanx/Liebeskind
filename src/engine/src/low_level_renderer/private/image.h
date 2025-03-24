#pragma once

#include <optional>
#include <vulkan/vulkan.hpp>

namespace Image {
std::tuple<vk::Image, vk::DeviceMemory> createImage(
    const vk::Device& device,
    const vk::PhysicalDevice& physicalDevice,
    uint32_t width,
    uint32_t height,
    vk::Format format,
    vk::ImageTiling tiling,
    vk::ImageUsageFlags usage,
    vk::MemoryPropertyFlags properties,
    uint32_t mipLevels = 1
);
void generateMipMaps(
    vk::Device device,
    vk::CommandPool commandPool,
    vk::Queue graphicsQueue,
    vk::Image image,
    uint32_t width,
    uint32_t height,
    uint32_t mipLevels
);
void copyBufferToImage(
    const vk::Device& device,
    const vk::CommandPool& commandPool,
    const vk::Queue& graphicsQueue,
    const vk::Buffer& sourceBuffer,
    const vk::Image& destinationImage,
    uint32_t width,
    uint32_t height
);
void transitionImageLayout(
    const vk::Device& device,
    const vk::CommandPool& commandPool,
    const vk::Queue& graphicsQueue,
    const vk::Image& image,
    vk::Format format,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    uint32_t mipLevels
);
vk::ImageView createImageView(
    const vk::Device& device,
    const vk::Image& image,
    vk::Format imageFormat,
    vk::ImageAspectFlags imageAspect
);
std::optional<vk::Format> findSupportedFormat(
    const vk::PhysicalDevice& physicalDevice,
    const std::vector<vk::Format>& candidates,
    vk::ImageTiling imageTiling,
    vk::FormatFeatureFlags features
);
bool hasStencilComponent(vk::Format format);
}  // namespace Image

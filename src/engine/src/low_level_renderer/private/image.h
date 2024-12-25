#pragma once

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
    vk::MemoryPropertyFlags properties
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
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout
);
}  // namespace Image

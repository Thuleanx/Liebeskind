#pragma once

#include <optional>
#include <vulkan/vulkan.hpp>

namespace Image {
struct CreateInfo {
    const vk::Device& device;
    const vk::PhysicalDevice& physicalDevice;
    vk::Extent3D size;
    vk::Format format;
    vk::ImageTiling tiling = vk::ImageTiling::eOptimal;
    vk::ImageUsageFlags usage;
    vk::MemoryPropertyFlags properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
    vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1;
    uint32_t mipLevels = 1;
};
std::tuple<vk::Image, vk::DeviceMemory> create(const CreateInfo& info);

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
    vk::ImageViewType viewType,
    vk::Format imageFormat,
    vk::ImageAspectFlags imageAspect,
    uint32_t mipBaseLevel,
    uint32_t mipLevels
);
std::optional<vk::Format> findSupportedFormat(
    const vk::PhysicalDevice& physicalDevice,
    std::span<const vk::Format> candidates,
    vk::ImageTiling imageTiling,
    vk::FormatFeatureFlags features
);
bool hasStencilComponent(vk::Format format);
}  // namespace Image

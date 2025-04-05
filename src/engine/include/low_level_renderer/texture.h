#pragma once

#include <vulkan/vulkan.hpp>

#include "low_level_renderer/descriptor_write_buffer.h"

namespace graphics {

struct TextureID {
    uint32_t index;
};

struct Texture {
    vk::Image image;
    vk::ImageView imageView;
    vk::DeviceMemory memory;
    vk::Format format;
    uint32_t mipLevels;
};

struct TextureStorage {
    std::vector<Texture> data;
};

Texture loadTextureFromFile(
    const char* filePath,
    vk::Device device,
    vk::PhysicalDevice physicalDevice,
    vk::CommandPool commandPool,
    vk::Queue graphicsQueue
);

Texture createTexture(
    vk::Device device,
    vk::PhysicalDevice physicalDevice,
    vk::Format format,
    uint32_t width,
    uint32_t height,
    vk::ImageTiling tiling,
    vk::ImageUsageFlags usage,
    vk::ImageAspectFlags aspect,
    vk::SampleCountFlagBits samplesCount
);

TextureID pushTextureFromFile(
    TextureStorage& textureStorage,
    const char* filePath,
    vk::Device device,
    vk::PhysicalDevice physicalDevice,
    vk::CommandPool commandPool,
    vk::Queue graphicsQueue,
    vk::Format imageFormat
);

void bindTextureToDescriptor(
    const TextureStorage& textureStorage,
    TextureID texture,
    vk::DescriptorSet descriptorSet,
    int binding,
    vk::Sampler sampler,
    DescriptorWriteBuffer& writeBuffer
);

void transitionLayout(
    const Texture& texture,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    vk::Device device,
    vk::CommandPool commandPool,
    vk::Queue graphicsQueue
);

void destroy(TextureStorage& textures, vk::Device device);
void destroy(std::span<const Texture> textures, vk::Device device);

}  // namespace graphics

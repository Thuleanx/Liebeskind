#pragma once

#include <vulkan/vulkan.hpp>

#include "low_level_renderer/descriptor_write_buffer.h"

namespace graphics {

struct TextureID {
    uint32_t index;
};

enum class TextureFormatHint {
    eLinear8,
    eGamma8,
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

vk::Format getIdealTextureFormat(int channels, const TextureFormatHint& hint);

Texture loadTextureFromFile(
    std::string_view filePath,
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
    std::string_view filePath,
    vk::Device device,
    vk::PhysicalDevice physicalDevice,
    vk::CommandPool commandPool,
    vk::Queue graphicsQueue,
    TextureFormatHint formatHint
);

void bindTextureToDescriptor(
    const TextureStorage& textureStorage,
    TextureID texture,
    vk::DescriptorSet descriptorSet,
    int binding,
    vk::Sampler sampler,
    DescriptorWriteBuffer& writeBuffer
);

void bindTextureToDescriptor(
    vk::ImageView imageView,
	vk::DescriptorSet descriptorSet,
	int binding,
	vk::Sampler sampler,
	DescriptorWriteBuffer& writeBuffer,
    vk::ImageLayout imageLayout
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

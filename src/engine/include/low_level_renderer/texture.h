#pragma once

#include <vulkan/vulkan.hpp>

#include "low_level_renderer/sampler.h"

class Texture {
   public:
    [[nodiscard]]
    static Texture load(
        const char* filePath,
        const vk::Device& device,
        const vk::PhysicalDevice& physicalDevice,
        vk::CommandPool& commandPool,
        vk::Queue& graphicsQueue
    );

    [[nodiscard]]
    static Texture create(
        const vk::Device& device,
        const vk::PhysicalDevice& physicalDevice,
        vk::Format format,
        uint32_t width,
        uint32_t height,
        vk::ImageTiling tiling,
        vk::ImageUsageFlags usage,
        vk::ImageAspectFlags aspect
    );

    void transitionLayout(
        const vk::Device& device,
        vk::CommandPool& commandPool,
        vk::Queue& graphicsQueue,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout
    );

    vk::DescriptorImageInfo getDescriptorImageInfo(const Sampler& sampler
    ) const;

    vk::Format getFormat() const;
    void destroyBy(const vk::Device& device);

   private:
    Texture(
        vk::Image image,
        vk::DeviceMemory deviceMemory,
        vk::ImageView imageView,
        vk::Format format
    );

   private:
    vk::Image image;
    vk::ImageView imageView;
    vk::DeviceMemory memory;
    vk::Format format;

    friend class GraphicsDeviceInterface;
};

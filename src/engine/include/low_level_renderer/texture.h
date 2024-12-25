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

    vk::DescriptorImageInfo getDescriptorImageInfo(const Sampler& sampler
    ) const;

    void destroyBy(const vk::Device& device);

   private:
    Texture(
        vk::Image image, vk::DeviceMemory deviceMemory, vk::ImageView imageView
    );

   private:
    vk::Image image;
    vk::ImageView imageView;
    vk::DeviceMemory memory;
};

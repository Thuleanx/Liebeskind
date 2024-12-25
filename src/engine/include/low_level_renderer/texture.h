#pragma once

#include <vulkan/vulkan.hpp>

class Texture {
   public:
    [[nodiscard]]
    static Texture load(
        const char* filePath,
        const vk::Device& device,
        const vk::PhysicalDevice& physicalDevice,
        vk::CommandPool &commandPool,
        vk::Queue& graphicsQueue
    );

    void destroyBy(const vk::Device& device);

   private:
    Texture(vk::Image image, vk::DeviceMemory deviceMemory);

   private:
    vk::Image image;
    vk::DeviceMemory memory;
};

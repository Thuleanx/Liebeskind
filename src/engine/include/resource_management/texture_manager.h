#pragma once

#include <vulkan/vulkan.hpp>

#include "low_level_renderer/descriptor_write_buffer.h"
#include "low_level_renderer/texture.h"

struct TextureID {
    uint32_t index;
};

class TextureManager {
   public:
    [[nodiscard]]
    TextureID load(
        const char* filePath,
        vk::Device device,
        vk::PhysicalDevice physicalDevice,
        vk::CommandPool commandPool,
        vk::Queue graphicsQueue
    );
    void bind(
        TextureID texture,
        vk::DescriptorSet descriptorSet,
        int binding,
        vk::Sampler sampler,
        graphics::DescriptorWriteBuffer& writeBuffer
    ) const;
    void destroyBy(vk::Device device);

   private:
    std::vector<graphics::Texture> textures;
};

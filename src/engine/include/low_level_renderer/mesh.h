#pragma once

#include <vulkan/vulkan.hpp>

#include "low_level_renderer/texture.h"
#include "low_level_renderer/vertex_buffer.h"

class Mesh {
   public:
    static Mesh load(
        const vk::Device& device,
        const vk::PhysicalDevice& physicalDevice,
        const vk::CommandPool& commandPool,
        const vk::Queue& graphicsQueue,
        const char* modelFilePath,
        const char* textureFilePath
    );
    void draw(const vk::CommandBuffer& commandBuffer) const;
    void bind(const vk::CommandBuffer& commandBuffer) const;
    void destroyBy(const vk::Device& device);

   private:
    Mesh(VertexBuffer vertices, Texture albedo);
   private:
    VertexBuffer vertices;
    Texture albedo;

    friend class GraphicsDeviceInterface;
};

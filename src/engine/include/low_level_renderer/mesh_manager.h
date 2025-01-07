#pragma once

#include <vulkan/vulkan.hpp>

#include "low_level_renderer/vertex_buffer.h"

// TODO: improve with generation counter
struct MeshID {
    uint32_t index;
};

struct Mesh {
    VertexBuffer vertexBuffer;
};

class MeshManager {
   public:
    MeshID load(
        const vk::Device &device,
        const vk::PhysicalDevice &physicalDevice,
        const vk::CommandPool &commandPool,
        const vk::Queue &graphicsQueue,
        const char *meshFilePath
    );

    void bind(vk::CommandBuffer commandBuffer, MeshID mesh);
    void draw(vk::CommandBuffer commandBuffer, MeshID mesh);
    void destroyBy(vk::Device device);

   private:
    std::vector<Mesh> meshes;
};

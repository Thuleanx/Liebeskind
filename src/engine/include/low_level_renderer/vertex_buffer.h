#pragma once

#include <array>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <optional>
#include <vector>
#include <vulkan/vulkan.hpp>

struct Vertex {
    glm::vec2 position;
    glm::vec3 color;

    static vk::VertexInputBindingDescription getBindingDescription();
    static std::array<vk::VertexInputAttributeDescription, 2>
    getAttributeDescriptions();
};

class VertexBuffer {
   public:
    static VertexBuffer create(
        const vk::Device& device,
        const vk::PhysicalDevice& physicalDevice,
        const vk::CommandPool& commandPool,
        const vk::Queue& graphicsQueue
    );
    void bind(const vk::CommandBuffer& commandBuffer) const;
    uint32_t getNumberOfVertices() const;
    void destroyBy(const vk::Device& device);

   private:
    VertexBuffer(vk::Buffer buffer, vk::DeviceMemory memory);

   private:
    vk::Buffer buffer;
    vk::DeviceMemory memory;
};

extern const std::vector<Vertex> triangleVertices;

#pragma once

#include <array>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
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
    void draw(const vk::CommandBuffer& commandBuffer) const;
    void destroyBy(const vk::Device& device);

   private:
    VertexBuffer(vk::Buffer buffer, vk::DeviceMemory memory, vk::Buffer indexBuffer, vk::DeviceMemory indexMemory);
    uint32_t getNumberOfVertices() const;
    uint32_t getNumberOfIndices() const;

   private:
    vk::Buffer vertexBuffer;
    vk::DeviceMemory vertexMemory;
    vk::Buffer indexBuffer;
    vk::DeviceMemory indexMemory;
};

extern const std::vector<Vertex> quadVertices;
extern const std::vector<uint16_t> quadIndices;

#pragma once

#include <array>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 texCoord;

   public:
    static vk::VertexInputBindingDescription getBindingDescription();
    static std::array<vk::VertexInputAttributeDescription, 4>
    getAttributeDescriptions();

    bool operator==(const Vertex& other) const;
};

struct VertexBuffer {
    vk::Buffer vertexBuffer;
    vk::DeviceMemory vertexMemory;
    vk::Buffer indexBuffer;
    vk::DeviceMemory indexMemory;
    uint32_t numberOfVertices;
    uint32_t numberOfIndices;

   public:
    static VertexBuffer create(
        const char* filePath,
        const vk::Device& device,
        const vk::PhysicalDevice& physicalDevice,
        const vk::CommandPool& commandPool,
        const vk::Queue& graphicsQueue
    );
    void bind(const vk::CommandBuffer& commandBuffer) const;
    void draw(const vk::CommandBuffer& commandBuffer) const;
    void destroyBy(const vk::Device& device) const;

   private:
    VertexBuffer(
        vk::Buffer vertexBuffer,
        vk::DeviceMemory vertexMemory,
        vk::Buffer indexBuffer,
        vk::DeviceMemory indexMemory,
        uint32_t numberOfVertices,
        uint32_t numberOfIndices
    ) :
        vertexBuffer(vertexBuffer),
        vertexMemory(vertexMemory),
        indexBuffer(indexBuffer),
        indexMemory(indexMemory),
        numberOfVertices(numberOfVertices),
        numberOfIndices(numberOfIndices) {}
};

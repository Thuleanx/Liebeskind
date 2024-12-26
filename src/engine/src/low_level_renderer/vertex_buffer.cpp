#include "low_level_renderer/vertex_buffer.h"

#include <cstddef>

#include "private/buffer.h"

const std::vector<Vertex> quadVertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};

const std::vector<uint16_t> quadIndices = {0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4};

std::array<vk::VertexInputAttributeDescription, 3>
Vertex::getAttributeDescriptions() {
    static std::array<vk::VertexInputAttributeDescription, 3>
        attributeDescriptions = {
            vk::VertexInputAttributeDescription(
                0,  // location
                0,  // binding
                vk::Format::eR32G32B32Sfloat,
                offsetof(Vertex, position)
            ),
            vk::VertexInputAttributeDescription(
                1,  // location
                0,  // binding
                vk::Format::eR32G32B32Sfloat,
                offsetof(Vertex, color)
            ),
            vk::VertexInputAttributeDescription(
                2,  // location
                0,  // binding
                vk::Format::eR32G32Sfloat,
                offsetof(Vertex, texCoord)
            )
        };
    return attributeDescriptions;
}

vk::VertexInputBindingDescription Vertex::getBindingDescription() {
    static vk::VertexInputBindingDescription bindingDescription(
        0, sizeof(Vertex), vk::VertexInputRate::eVertex
    );
    return bindingDescription;
}

VertexBuffer::VertexBuffer(
    vk::Buffer vertexBuffer,
    vk::DeviceMemory vertexMemory,
    vk::Buffer indexBuffer,
    vk::DeviceMemory indexMemory
) :
    vertexBuffer(vertexBuffer),
    vertexMemory(vertexMemory),
    indexBuffer(indexBuffer),
    indexMemory(indexMemory) {}

VertexBuffer VertexBuffer::create(
    const vk::Device& device,
    const vk::PhysicalDevice& physicalDevice,
    const vk::CommandPool& commandPool,
    const vk::Queue& graphicsQueue
) {
    auto [vertexBuffer, deviceMemory] = Buffer::loadToBuffer(
        device,
        physicalDevice,
        commandPool,
        graphicsQueue,
        quadVertices,
        vk::BufferUsageFlagBits::eVertexBuffer
    );

    auto [indexBuffer, indexDeviceMemory] = Buffer::loadToBuffer(
        device,
        physicalDevice,
        commandPool,
        graphicsQueue,
        quadIndices,
        vk::BufferUsageFlagBits::eIndexBuffer
    );

    return VertexBuffer(
        vertexBuffer, deviceMemory, indexBuffer, indexDeviceMemory
    );
}

void VertexBuffer::destroyBy(const vk::Device& device) {
    device.destroyBuffer(vertexBuffer);
    device.freeMemory(vertexMemory);
    device.destroyBuffer(indexBuffer);
    device.freeMemory(indexMemory);
}

void VertexBuffer::bind(const vk::CommandBuffer& commandBuffer) const {
    vk::DeviceSize offsets[] = {0};
    commandBuffer.bindVertexBuffers(0, 1, &vertexBuffer, offsets);
    commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint16);
}

void VertexBuffer::draw(const vk::CommandBuffer& commandBuffer) const {
    commandBuffer.drawIndexed(getNumberOfIndices(), 1, 0, 0, 0);
}

uint32_t VertexBuffer::getNumberOfVertices() const {
    return static_cast<uint32_t>(quadVertices.size());
}

uint32_t VertexBuffer::getNumberOfIndices() const {
    return static_cast<uint32_t>(quadIndices.size());
}

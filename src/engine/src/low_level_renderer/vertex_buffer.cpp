#include "low_level_renderer/vertex_buffer.h"

#include <cstddef>
#include <cstring>

#include "private/buffer.h"
#include "private/helpful_defines.h"

const std::vector<Vertex> triangleVertices = {
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};

std::array<vk::VertexInputAttributeDescription, 2>
Vertex::getAttributeDescriptions() {
    static std::array<vk::VertexInputAttributeDescription, 2>
        attributeDescriptions = {
            vk::VertexInputAttributeDescription(
                0,  // location
                0,  // binding
                vk::Format::eR32G32Sfloat,
                offsetof(Vertex, position)
            ),
            vk::VertexInputAttributeDescription(
                1,  // location
                0,  // binding
                vk::Format::eR32G32B32Sfloat,
                offsetof(Vertex, color)
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

VertexBuffer::VertexBuffer(vk::Buffer buffer, vk::DeviceMemory memory) :
    buffer(buffer), memory(memory) {}

VertexBuffer VertexBuffer::create(
    const vk::Device& device,
    const vk::PhysicalDevice& physicalDevice,
    const vk::CommandPool& commandPool,
    const vk::Queue& graphicsQueue
) {
    const uint32_t bufferSize =
        sizeof(triangleVertices[0]) * triangleVertices.size();

    const auto [stagingBuffer, stagingBufferMemory] = Buffer::create(
        device,
        physicalDevice,
        bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent
    );

    {  // memcpy the vertex data onto staging buffer
        const vk::ResultValue<void*> mappedMemory =
            device.mapMemory(stagingBufferMemory, 0, bufferSize, {});
        VULKAN_ENSURE_SUCCESS(
            mappedMemory.result, "Can't map staging buffer memory"
        );
        memcpy(
            mappedMemory.value,
            triangleVertices.data(),
            static_cast<size_t>(bufferSize)
        );
        device.unmapMemory(stagingBufferMemory);
    }

    const auto [vertexBuffer, deviceMemory] = Buffer::create(
        device,
        physicalDevice,
        bufferSize,
        vk::BufferUsageFlagBits::eVertexBuffer |
            vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    Buffer::copyBuffer(
        device,
        commandPool,
        graphicsQueue,
        stagingBuffer,
        vertexBuffer,
        bufferSize
    );

    device.destroyBuffer(stagingBuffer);
    device.freeMemory(stagingBufferMemory);

    return VertexBuffer(vertexBuffer, deviceMemory);
}

void VertexBuffer::destroyBy(const vk::Device& device) {
    device.destroyBuffer(buffer);
    device.freeMemory(memory);
}

void VertexBuffer::bind(const vk::CommandBuffer& commandBuffer) const {
    vk::DeviceSize offsets[] = {0};
    commandBuffer.bindVertexBuffers(0, 1, &buffer, offsets);
}

uint32_t VertexBuffer::getNumberOfVertices() const {
    return static_cast<uint32_t>(triangleVertices.size());
}

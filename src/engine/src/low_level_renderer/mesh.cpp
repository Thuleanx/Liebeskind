#include "low_level_renderer/mesh.h"

Mesh Mesh::load(
    const vk::Device &device,
    const vk::PhysicalDevice &physicalDevice,
    const vk::CommandPool &commandPool,
    const vk::Queue& graphicsQueue,
    const char *modelFilePath,
    const char *textureFilePath
) {
    VertexBuffer vertexBuffer = VertexBuffer::create(
        modelFilePath, device, physicalDevice, commandPool, graphicsQueue
    );
    Texture albedo = Texture::load(
        textureFilePath,
        device,
        physicalDevice,
        commandPool,
        graphicsQueue
    );

    return Mesh(vertexBuffer, albedo);
}

Mesh::Mesh(VertexBuffer vertices, Texture texture) :
    vertices(vertices), albedo(texture) {}

void Mesh::draw(const vk::CommandBuffer& commandBuffer) const {
    vertices.draw(commandBuffer);
}

void Mesh::bind(const vk::CommandBuffer& commandBuffer) const {
    vertices.bind(commandBuffer);
}

void Mesh::destroyBy(const vk::Device& device) {
    vertices.destroyBy(device);
    albedo.destroyBy(device);
}

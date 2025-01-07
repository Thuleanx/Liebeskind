#include "low_level_renderer/vertex_buffer.h"

#include <tiny_obj_loader.h>

#include <cstddef>
#include <glm/gtx/hash.hpp>

#include "logger/assert.h"
#include "private/buffer.h"

namespace std {
template <>
struct hash<Vertex> {
    size_t operator()(Vertex const& vertex) const {
        size_t hash_value = 0;
        hash_value = (hash_value << 1) ^ hash<glm::vec3>()(vertex.position);
        hash_value = (hash_value << 1) ^ hash<glm::vec3>()(vertex.normal);
        hash_value = (hash_value << 1) ^ hash<glm::vec3>()(vertex.color);
        hash_value = (hash_value << 1) ^ hash<glm::vec2>()(vertex.texCoord);
        return hash_value;
    }
};
}  // namespace std

bool Vertex::operator==(const Vertex& other) const {
    return position == other.position && color == other.color &&
           texCoord == other.texCoord;
}

std::array<vk::VertexInputAttributeDescription, 4>
Vertex::getAttributeDescriptions() {
    static std::array<vk::VertexInputAttributeDescription, 4>
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
                offsetof(Vertex, normal)
            ),
            vk::VertexInputAttributeDescription(
                2,  // location
                0,  // binding
                vk::Format::eR32G32B32Sfloat,
                offsetof(Vertex, color)
            ),
            vk::VertexInputAttributeDescription(
                3,  // location
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

VertexBuffer VertexBuffer::create(
    const char* filePath,
    const vk::Device& device,
    const vk::PhysicalDevice& physicalDevice,
    const vk::CommandPool& commandPool,
    const vk::Queue& graphicsQueue
) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    bool successfullyLoadedModel =
        tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath);
    ASSERT(
        successfullyLoadedModel,
        "Can't load model at " << filePath << " " << warn << " " << err
    );

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    {  // populate vertices and indices arrays
        std::unordered_map<Vertex, uint32_t> unique_vertices;
        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                const Vertex vertex{
                    .position =
                        glm::vec3{
                            attrib.vertices[3 * index.vertex_index + 0],
                            attrib.vertices[3 * index.vertex_index + 1],
                            attrib.vertices[3 * index.vertex_index + 2]
                        },
                    .normal =
                        glm::vec3{
                            attrib.normals[3 * index.normal_index + 0],
                            attrib.normals[3 * index.normal_index + 1],
                            attrib.normals[3 * index.normal_index + 2]
                        },
                    .color = glm::vec3{1.0, 1.0, 1.0},
                    // obj format coordinate system makes 0 the bottom of the
                    // image, which is different from vulkan which considers it
                    // the top
                    .texCoord =
                        glm::vec2{
                            attrib.texcoords[2 * index.texcoord_index],
                            1.0 - attrib.texcoords[2 * index.texcoord_index + 1]
                        }
                };

                if (!unique_vertices.count(vertex)) {
                    unique_vertices[vertex] =
                        static_cast<uint32_t>(unique_vertices.size());
                    vertices.push_back(vertex);
                }
                indices.push_back(unique_vertices[vertex]);
            }
        }
    }

    auto [vertexBuffer, deviceMemory] = Buffer::loadToBuffer(
        device,
        physicalDevice,
        commandPool,
        graphicsQueue,
        vertices,
        vk::BufferUsageFlagBits::eVertexBuffer
    );

    auto [indexBuffer, indexDeviceMemory] = Buffer::loadToBuffer(
        device,
        physicalDevice,
        commandPool,
        graphicsQueue,
        indices,
        vk::BufferUsageFlagBits::eIndexBuffer
    );

    return VertexBuffer(
        vertexBuffer,
        deviceMemory,
        indexBuffer,
        indexDeviceMemory,
        vertices.size(),
        indices.size()
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
    commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);
}

void VertexBuffer::draw(const vk::CommandBuffer& commandBuffer) const {
    commandBuffer.drawIndexed(getNumberOfIndices(), 1, 0, 0, 0);
}

uint32_t VertexBuffer::getNumberOfVertices() const {
    return static_cast<uint32_t>(numberOfVertices);
}

uint32_t VertexBuffer::getNumberOfIndices() const {
    return static_cast<uint32_t>(numberOfIndices);
}

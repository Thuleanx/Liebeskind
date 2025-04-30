#pragma once

#include <array>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan.hpp>

namespace graphics {
using IndexType = uint32_t;

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec3 color;
	glm::vec2 texCoord;

   public:
	constexpr static vk::VertexInputBindingDescription getBindingDescription() {
		return vk::VertexInputBindingDescription(
			0, sizeof(Vertex), vk::VertexInputRate::eVertex
		);
	}

	static std::array<vk::VertexInputAttributeDescription, 5>
	getAttributeDescriptions();

	bool operator==(const Vertex& other) const {
		return position == other.position && color == other.color &&
			   normal == other.normal && texCoord == other.texCoord &&
			   tangent == other.tangent;
	}
};

glm::vec3 getTangent(
	const graphics::Vertex& v0,
	const graphics::Vertex& v1,
	const graphics::Vertex& v2
);

struct VertexBuffer {
	vk::Buffer vertexBuffer;
	vk::DeviceMemory vertexMemory;
	vk::Buffer indexBuffer;
	vk::DeviceMemory indexMemory;
	uint32_t numberOfVertices;
	uint32_t numberOfIndices;

   public:
	static VertexBuffer create(
		std::string_view filePath,
		const vk::Device& device,
		const vk::PhysicalDevice& physicalDevice,
		const vk::CommandPool& commandPool,
		const vk::Queue& graphicsQueue
	);

	static VertexBuffer create(
		const std::vector<Vertex>& vertices,
		const std::vector<IndexType>& indices,
		vk::Device device,
		vk::PhysicalDevice physicalDevice,
		vk::CommandPool commandPool,
		vk::Queue graphicsQueue
	);
};

void bind(vk::CommandBuffer commandBuffer, const VertexBuffer& vertexBuffer);
void drawVertices(
	vk::CommandBuffer commandBuffer,
	const VertexBuffer& vertexBuffer,
	uint16_t instanceCount = 1
);
void destroy(std::span<const VertexBuffer> vertexBuffers, vk::Device device);

};	// namespace graphics

namespace std {
template <>
struct hash<graphics::Vertex> {
	size_t operator()(graphics::Vertex const& vertex) const;
};
}  // namespace std

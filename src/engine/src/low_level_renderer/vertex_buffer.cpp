#include "low_level_renderer/vertex_buffer.h"

#include <tiny_obj_loader.h>

#include <cstddef>
#include <glm/gtx/hash.hpp>

#include "core/logger/assert.h"
#include "glm/gtx/hash.hpp"
#include "private/buffer.h"

namespace std {
template <>
struct hash<graphics::Vertex> {
	size_t operator()(graphics::Vertex const& vertex) const {
		size_t hash_value = 0;
		hash_value = (hash_value << 1) ^ hash<glm::vec3>()(vertex.position);
		hash_value = (hash_value << 1) ^ hash<glm::vec3>()(vertex.normal);
		hash_value = (hash_value << 1) ^ hash<glm::vec3>()(vertex.color);
		hash_value = (hash_value << 1) ^ hash<glm::vec2>()(vertex.texCoord);
		return hash_value;
	}
};
}  // namespace std

namespace graphics {
std::array<vk::VertexInputAttributeDescription, 5>
Vertex::getAttributeDescriptions() {
	static std::array<vk::VertexInputAttributeDescription, 5>
		attributeDescriptions = {
			vk::VertexInputAttributeDescription(
				0,	// location
				0,	// binding
				vk::Format::eR32G32B32Sfloat,
				offsetof(Vertex, position)
			),
			vk::VertexInputAttributeDescription(
				1,	// location
				0,	// binding
				vk::Format::eR32G32B32Sfloat,
				offsetof(Vertex, normal)
			),
			vk::VertexInputAttributeDescription(
				2,	// location
				0,	// binding
				vk::Format::eR32G32B32Sfloat,
				offsetof(Vertex, tangent)
			),
			vk::VertexInputAttributeDescription(
				3,	// location
				0,	// binding
				vk::Format::eR32G32B32Sfloat,
				offsetof(Vertex, color)
			),
			vk::VertexInputAttributeDescription(
				4,	// location
				0,	// binding
				vk::Format::eR32G32Sfloat,
				offsetof(Vertex, texCoord)
			)
		};
	return attributeDescriptions;
}

VertexBuffer VertexBuffer::create(
    std::string_view filePath,
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
		tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath.data());
	ASSERT(
		successfullyLoadedModel,
		"Can't load model at " << filePath << " " << warn << " " << err
	);

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	{  // populate vertices and indices arrays
		std::unordered_map<Vertex, uint32_t> unique_vertices;
		for (const auto& shape : shapes) {
			size_t indexBegin = indices.size();

			ASSERT(
				shape.mesh.num_face_vertices[0] == 3,
				"We currently only support triangle meshes"
			);

			for (size_t poly = 0, indexOffset = 0;
				 poly < shape.mesh.num_face_vertices.size();
				 indexOffset += shape.mesh.num_face_vertices[poly++]) {
				ASSERT(
					shape.mesh.num_face_vertices[poly] == 3,
					"Cannot support meshes with polygons of "
						<< shape.mesh.num_face_vertices[poly]
						<< " vertices, only triangle meshes permitted"
				);

				for (size_t faceIndex = 0;
					 faceIndex < shape.mesh.num_face_vertices[poly];
					 faceIndex++) {
					const tinyobj::index_t index =
						shape.mesh.indices[indexOffset + faceIndex];
					ASSERT(
						index.normal_index >= 0,
						"Vertex does not specify a normal"
					);
					ASSERT(
						index.texcoord_index >= 0,
						"Vertex does not specify a tex coordinate"
					);
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
						.tangent = glm::vec3(0),
						.color = glm::vec3{1.0, 1.0, 1.0},
						// obj format coordinate system makes 0 the bottom of
						// the image, which is different from vulkan which
						// considers it the top
						.texCoord =
							glm::vec2{
								attrib.texcoords[2 * index.texcoord_index],
								1.0 -
									attrib
										.texcoords[2 * index.texcoord_index + 1]
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

			constexpr int NUM_OF_VERTS_PER_TRIANGLE = 3;

			for (size_t i = indexBegin; i < indices.size();
				 i += NUM_OF_VERTS_PER_TRIANGLE) {
				Vertex& v0 = vertices[indices[i]];
				Vertex& v1 = vertices[indices[i + 1]];
				Vertex& v2 = vertices[indices[i + 2]];

				glm::vec3 edge01 = v1.position - v0.position;
				glm::vec3 edge02 = v2.position - v0.position;

				glm::vec2 deltaUV01 = v1.texCoord - v0.texCoord;
				glm::vec2 deltaUV02 = v2.texCoord - v0.texCoord;

				float normalizer = 1.0f / (deltaUV01.x * deltaUV02.y -
										   deltaUV01.y * deltaUV02.x);

				glm::vec3 tangent =
					(deltaUV02.y * edge01 - deltaUV01.y * edge02) * normalizer;

				for (int k = 0; k < NUM_OF_VERTS_PER_TRIANGLE; k++)
					vertices[indices[i + k]].tangent += tangent;
			}
		}

		for (size_t i = 0; i < vertices.size(); i++)
			vertices[i].tangent = glm::normalize(vertices[i].tangent);
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

	return VertexBuffer{
		vertexBuffer,
		deviceMemory,
		indexBuffer,
		indexDeviceMemory,
		static_cast<uint32_t>(vertices.size()),
		static_cast<uint32_t>(indices.size())
	};
}

void bind(vk::CommandBuffer commandBuffer, const VertexBuffer& vertexBuffer) {
	vk::DeviceSize offsets[] = {0};
	commandBuffer.bindVertexBuffers(0, 1, &vertexBuffer.vertexBuffer, offsets);
	commandBuffer.bindIndexBuffer(
		vertexBuffer.indexBuffer, 0, vk::IndexType::eUint32
	);
}

void drawVertices(
	vk::CommandBuffer commandBuffer,
	const VertexBuffer& vertexBuffer,
	uint16_t instanceCount
) {
	commandBuffer.drawIndexed(
		vertexBuffer.numberOfIndices, instanceCount, 0, 0, 0
	);
}

void destroy(std::span<const VertexBuffer> vertexBuffers, vk::Device device) {
	for (const VertexBuffer& vertexBuffer : vertexBuffers) {
		device.destroyBuffer(vertexBuffer.vertexBuffer);
		device.freeMemory(vertexBuffer.vertexMemory);
		device.destroyBuffer(vertexBuffer.indexBuffer);
		device.freeMemory(vertexBuffer.indexMemory);
	}
}
};	// namespace graphics

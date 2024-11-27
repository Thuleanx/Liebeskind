#pragma once

#include <array>
#include <vulkan/vulkan.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vector>
#include <optional>

struct Vertex {
	glm::vec2 position;
	glm::vec3 color;

	static vk::VertexInputBindingDescription getBindingDescription();
	static std::array<vk::VertexInputAttributeDescription, 2>
	getAttributeDescriptions();
};

class VertexBuffer {
public:
	static VertexBuffer create(const vk::Device& device,
							   const vk::PhysicalDevice& physicalDevice);
    void bind(const vk::CommandBuffer& commandBuffer) const;
    uint32_t getNumberOfVertices() const;
	void destroyBy(const vk::Device& device);
private:
	VertexBuffer(vk::Buffer buffer, vk::DeviceMemory memory);

	static std::optional<uint32_t> findSuitableMemoryType(
        const vk::PhysicalDeviceMemoryProperties& memoryProperties,
        const uint32_t typeFilter,
		const vk::MemoryPropertyFlags properties
    );
private:
	vk::Buffer buffer;
	vk::DeviceMemory memory;
};

extern const std::vector<Vertex> triangleVertices;

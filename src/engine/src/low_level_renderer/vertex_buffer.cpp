#include "low_level_renderer/vertex_buffer.h"
#include "private/helpful_defines.h"
#include <cstddef>
#include <cstring>
#include <optional>

const std::vector<Vertex> triangleVertices = {
	{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
	{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};

std::array<vk::VertexInputAttributeDescription, 2>
Vertex::getAttributeDescriptions() {
	static std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions
	= {
		vk::VertexInputAttributeDescription(
		0, // location
		0, // binding
		vk::Format::eR32G32Sfloat,
		offsetof(Vertex, position)
		),
		vk::VertexInputAttributeDescription(
		1, // location
		0, // binding
		vk::Format::eR32G32B32Sfloat,
		offsetof(Vertex, color)
		)
	};
	return attributeDescriptions;
}

vk::VertexInputBindingDescription Vertex::getBindingDescription() {
	static vk::VertexInputBindingDescription bindingDescription(
		0,
		sizeof(Vertex),
		vk::VertexInputRate::eVertex
	);
	return bindingDescription;
}

VertexBuffer::VertexBuffer(vk::Buffer buffer,
						   vk::DeviceMemory memory) : buffer(buffer), memory(memory) {}

VertexBuffer VertexBuffer::create(const vk::Device& device,
								  const vk::PhysicalDevice& physicalDevice) {
	const vk::BufferCreateInfo bufferInfo(
		{},
		sizeof(triangleVertices[0]) * triangleVertices.size(),
		vk::BufferUsageFlagBits::eVertexBuffer,
		vk::SharingMode::eExclusive
	);
	const vk::ResultValue<vk::Buffer> vertexBufferCreation = device.createBuffer(
			bufferInfo);
	VULKAN_ENSURE_SUCCESS(vertexBufferCreation.result,
						  "Can't create vertex buffer:");
	const vk::MemoryRequirements memoryRequirements =
		device.getBufferMemoryRequirements(vertexBufferCreation.value);
	const std::optional<uint32_t> memoryTypeIndex = findSuitableMemoryType(
			physicalDevice.getMemoryProperties(),
			memoryRequirements.memoryTypeBits,
			vk::MemoryPropertyFlagBits::eHostVisible |
			vk::MemoryPropertyFlagBits::eHostCoherent
		);
	ASSERT(memoryTypeIndex.has_value(), "Failed to find suitable memory type");
	const vk::MemoryAllocateInfo allocateInfo(
		memoryRequirements.size,
		memoryTypeIndex.value()
	);
	const vk::ResultValue<vk::DeviceMemory> deviceMemoryAllocation =
		device.allocateMemory(allocateInfo);
	VULKAN_ENSURE_SUCCESS(deviceMemoryAllocation.result,
						  "Failed to allocate vertex buffer memory");
	VULKAN_ENSURE_SUCCESS_EXPR(device.bindBufferMemory(vertexBufferCreation.value,
							   deviceMemoryAllocation.value, 0), 
                                "Failed to bind buffer memory");
	vk::ResultValue<void*> mappedMemory = device.mapMemory(
			deviceMemoryAllocation.value,
			0,
			bufferInfo.size,
			{});
	VULKAN_ENSURE_SUCCESS(mappedMemory.result, "Can't map vertex buffer memory");
	memcpy(mappedMemory.value, triangleVertices.data(),
		   static_cast<size_t>(bufferInfo.size));
	device.unmapMemory(deviceMemoryAllocation.value);
	return VertexBuffer(vertexBufferCreation.value, deviceMemoryAllocation.value);
}

std::optional<uint32_t> VertexBuffer::findSuitableMemoryType(
	const vk::PhysicalDeviceMemoryProperties& memoryProperties,
	const uint32_t typeFilter,
	const vk::MemoryPropertyFlags properties
) {
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
		bool doesPropFitTypeFilter = typeFilter & (1 << i);
		bool doesPropFitPropertyFlags = (memoryProperties.memoryTypes[i].propertyFlags &
		properties) == properties;

		if (doesPropFitTypeFilter && doesPropFitPropertyFlags)
			return i;
	}

	return {};
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

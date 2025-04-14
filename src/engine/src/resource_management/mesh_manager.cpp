#include "resource_management/mesh_manager.h"

#include "core/logger/assert.h"

MeshID MeshManager::load(
	const vk::Device &device,
	const vk::PhysicalDevice &physicalDevice,
	const vk::CommandPool &commandPool,
	const vk::Queue &graphicsQueue,
	const char *meshFilePath
) {
	graphics::VertexBuffer vertexBuffer = graphics::VertexBuffer::create(
		meshFilePath, device, physicalDevice, commandPool, graphicsQueue
	);
	MeshID id{static_cast<uint32_t>(meshes.size())};
	meshes.push_back({vertexBuffer});
	return id;
}

void MeshManager::bind(vk::CommandBuffer commandBuffer, MeshID mesh) const {
	ASSERT(
		mesh.index < meshes.size(), "Mesh id is invalid: index out of range"
	);
	graphics::bind(commandBuffer, meshes[mesh.index]);
}

void MeshManager::draw(
	vk::CommandBuffer commandBuffer, MeshID mesh, uint16_t instancesCount
) const {
	ASSERT(
		mesh.index < meshes.size(), "Mesh id is invalid: index out of range"
	);
	graphics::drawVertices(commandBuffer, meshes[mesh.index], instancesCount);
}

void MeshManager::destroyBy(vk::Device device) {
	graphics::destroy(meshes, device);
}

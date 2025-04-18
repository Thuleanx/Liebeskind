#include "low_level_renderer/meshes.h"

namespace graphics {

MeshStorage MeshStorage::create() {
	return {
		.meshes = {},
		.indices = algo::GenerationIndexArray<MAX_MESHES>::create()
	};
}

MeshID load(
	MeshStorage &storage,
	vk::Device device,
	vk::PhysicalDevice physicalDevice,
	vk::CommandPool commandPool,
	vk::Queue graphicsQueue,
	const char *meshFilePath
) {
	const algo::GenerationIndexPair index = algo::pushBack(storage.indices);
	const graphics::VertexBuffer vertexBuffer = graphics::VertexBuffer::create(
		meshFilePath, device, physicalDevice, commandPool, graphicsQueue
	);
	storage.meshes[index.index] = vertexBuffer;
	return {index};
}

void bind(
	const MeshStorage &storage, vk::CommandBuffer commandBuffer, MeshID mesh
) {
	ASSERT(
		mesh.index < storage.meshes.size(),
		"Mesh id is invalid: index (" << mesh.index << ") out of range [0, "
									  << storage.meshes.size() << ")"
	);
	ASSERT(
		algo::isIndexValid(storage.indices, mesh),
		"Binding a mesh with invalid mesh ID. Either this mesh has been "
		"deleted or the ID is ill-formed"
	);
	graphics::bind(commandBuffer, storage.meshes[mesh.index]);
}

void draw(
	const MeshStorage &storage,
	vk::CommandBuffer commandBuffer,
	MeshID mesh,
	uint16_t instanceCount
) {
	ASSERT(
		mesh.index < storage.meshes.size(),
		"Mesh id is invalid: index (" << mesh.index << ") out of range [0, "
									  << storage.meshes.size() << ")"
	);
	ASSERT(
		algo::isIndexValid(storage.indices, mesh),
		"Binding a mesh with invalid mesh ID. Either this mesh has been "
		"deleted or the ID is ill-formed"
	);
	graphics::drawVertices(
		commandBuffer, storage.meshes[mesh.index], instanceCount
	);
}

void unload(
	MeshStorage &storage,
	std::span<const algo::GenerationIndexPair> indices,
	vk::Device device
) {
	for (const algo::GenerationIndexPair &index : indices) {
		if (algo::isIndexValid(storage.indices, index)) {
			destroy({std::addressof(storage.meshes[index.index]), 1}, device);
		}
	}
	destroy(storage.indices, indices);
}

void destroy(const MeshStorage &storage, vk::Device device) {
	const std::vector<uint16_t> liveIndices =
		algo::getLiveIndices(storage.indices);

	const std::vector<VertexBuffer> liveVertices = [&]() {
		std::vector<VertexBuffer> result;
		result.reserve(liveIndices.size());
		for (uint16_t liveIndex : liveIndices) {
			result.push_back(storage.meshes[liveIndex]);
		}
		return result;
	}();

	destroy(liveVertices, device);
}

}  // namespace graphics

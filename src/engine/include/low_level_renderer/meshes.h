#pragma once

#include "core/algo/generation_index_array.h"
#include "low_level_renderer/vertex_buffer.h"

namespace graphics {
constexpr size_t MAX_MESHES = 1000;

using MeshID = algo::GenerationIndexPair;

struct MeshStorage {
	std::array<VertexBuffer, MAX_MESHES> meshes;
	algo::GenerationIndexArray<MAX_MESHES> indices;

   public:
	static MeshStorage create();
};

[[nodiscard]]
MeshID load(
	MeshStorage& storage,
	vk::Device device,
	vk::PhysicalDevice physicalDevice,
	vk::CommandPool commandPool,
	vk::Queue graphicsQueue,
    std::string_view meshFilePath
);

void bind(
	const MeshStorage& storage, vk::CommandBuffer commandBuffer, MeshID mesh
);

void draw(
	const MeshStorage& storage,
	vk::CommandBuffer commandBuffer,
	MeshID mesh,
	uint16_t instanceCount = 1
);

void unload(
	MeshStorage& storage,
	std::span<const algo::GenerationIndexPair> indices,
	vk::Device device
);

void destroy(const MeshStorage& storage, vk::Device device);

}  // namespace graphics

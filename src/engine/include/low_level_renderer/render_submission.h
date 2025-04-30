#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include "low_level_renderer/instance_rendering.h"
#include "low_level_renderer/material_pipeline.h"
#include "low_level_renderer/materials.h"
#include "low_level_renderer/meshes.h"

namespace graphics {
struct RenderObject {
	PipelineSpecializationConstants variant;
	glm::mat4 transform;
	MaterialInstanceID material;
	MeshID mesh;
};

struct InstancedRenderObject {
	PipelineSpecializationConstants variant;
	RenderInstanceID instance;
	MaterialInstanceID material;
	MeshID mesh;
	uint16_t count;
};

struct RenderSubmission {
	std::vector<RenderObject> renderObjects;
	std::vector<InstancedRenderObject> instances;
	std::vector<std::vector<InstanceData>> instanceData;

   public:
	static RenderSubmission create();
	void clear();
};

void submit(
	RenderSubmission& renderSubmission,
	std::span<const InstancedRenderObject> instances,
	std::span<const std::vector<InstanceData>> data
);

void submit(
	RenderSubmission& renderSubmission,
	std::span<const RenderObject> renderObjects
);

void prepForRecording(
	RenderSubmission& renderSubmission,
	const RenderInstanceManager& instanceManager,
	uint32_t currentFrame
);

void recordRegularDrawCalls(
	const RenderSubmission& renderSubmission,
	vk::CommandBuffer buffer,
	vk::PipelineLayout pipelineLayout,
	const MaterialPipeline& pipelines,
	const MaterialStorage& materials,
	const MeshStorage& meshes
);

void recordInstancedDrawCalls(
	const RenderSubmission& renderSubmission,
	vk::CommandBuffer buffer,
	vk::PipelineLayout pipelineLayout,
	const RenderInstanceManager& instanceManager,
	const MaterialPipeline& pipelines,
	const MaterialStorage& materials,
	const MeshStorage& meshes,
    uint32_t currentFrame
);

void clear(RenderSubmission& renderSubmission);

}  // namespace graphics

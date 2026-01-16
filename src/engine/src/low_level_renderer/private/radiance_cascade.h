#pragma once

#include "low_level_renderer/radiance_cascade.h"
#include "low_level_renderer/render_submission.h"

namespace graphics {

struct RadianceCascadeCreateInfo {
    const vk::Device& device;
    const vk::PhysicalDevice& physicalDevice;
    const vk::Sampler& sampler;
    DescriptorWriteBuffer& writeBuffer;
	ShaderStorage& shaders;
    uint32_t resolution;
    glm::vec3 center;
    glm::vec3 size;
};
RadianceCascadeData create(const RadianceCascadeCreateInfo& info);

void recordDraw(
    RadianceCascadeData& cascadeData,
    const RenderSubmission& renderSubmission,
    vk::CommandBuffer buffer,
	const RenderInstanceManager& instanceManager,
	const MaterialPipeline& pipelines,
	const MaterialStorage& materials,
	const MeshStorage& meshes,
	uint32_t currentFrame
);

void destroy(const RadianceCascadeData& radianceCascade, vk::Device device);

}

#pragma once

#include <vulkan/vulkan.hpp>

#include "low_level_renderer/descriptor_allocator.h"
#include "low_level_renderer/renderpass_data.h"

namespace graphics {
enum class MainPipelineDescriptorSetBindingPoint {
	eGlobal = 0,
	eMaterial = 1,
	eInstanceRendering = 2,
};

enum class PostProcessingDescriptorSetBindingPoint { eGlobal = 0 };

struct PipelineData {
	vk::Pipeline pipeline;
	vk::PipelineLayout layout;
};

struct PipelineDescriptorData {
	vk::DescriptorSetLayout setLayout;
	DescriptorAllocator allocator;
};

struct MaterialPipeline {
	PipelineData regularPipeline;
	PipelineData instanceRenderingPipeline;
	PipelineData postProcessingPipeline;
	PipelineDescriptorData globalDescriptor;
	PipelineDescriptorData instanceRenderingDescriptor;
	PipelineDescriptorData materialDescriptor;
	PipelineDescriptorData postProcessingDescriptor;

   public:
	static MaterialPipeline create(
		vk::Device device,
		const RenderPassData& renderPasses,
		vk::ShaderModule vertexShader,
		vk::ShaderModule instanceRenderingVertexShader,
		vk::ShaderModule fragmentShader,
		vk::ShaderModule postProcessingVertexShader,
		vk::ShaderModule postProcessingFragmentShader
	);
};

std::array<PipelineData, 2> createMainPipelines(
	vk::Device device,
	const RenderPassData& renderPasses,
	const PipelineDescriptorData& globalDescriptorData,
	const PipelineDescriptorData& materialDescriptorData,
	const PipelineDescriptorData& instanceRenderingDescriptorData,
	vk::ShaderModule vertexShader,
	vk::ShaderModule instanceRenderingVertexShader,
	vk::ShaderModule fragmentShader
);

std::array<PipelineData, 1> createPostProcessingPipelines(
	vk::Device device,
	const RenderPassData& renderPasses,
	const PipelineDescriptorData& postProcessingDescriptorData,
	vk::ShaderModule vertexShader,
	vk::ShaderModule fragmentShader
);

void destroy(const MaterialPipeline& pipeline, vk::Device device);
void destroy(const PipelineData& pipelineData, vk::Device device);
void destroy(
	const PipelineDescriptorData& pipelineDescriptor, vk::Device device
);
}  // namespace graphics

#pragma once

#include <unordered_map>
#include <vulkan/vulkan.hpp>

#include "low_level_renderer/descriptor_allocator.h"
#include "low_level_renderer/pipeline_template.h"
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
	using VariantMap = std::unordered_map<
		PipelineSpecializationConstants,
		vk::Pipeline,
		PipelineSpecializationConstantsHashFunction>;
    
	PipelineTemplate pipelineTemplate;
	VariantMap regularPipelineVariants;
	VariantMap instanceRenderingPipelineVariants;
	vk::PipelineLayout regularPipelineLayout;
	vk::PipelineLayout instanceRenderingPipelineLayout;
	PipelineData postProcessingPipeline;
	PipelineDescriptorData globalDescriptor;
	PipelineDescriptorData instanceRenderingDescriptor;
	PipelineDescriptorData materialDescriptor;
	PipelineDescriptorData postProcessingDescriptor;

   public:
	static MaterialPipeline create(
		vk::Device device,
		const RenderPassData& renderPasses,
		vk::ShaderModule postProcessingVertexShader,
		vk::ShaderModule postProcessingFragmentShader
	);
};

std::array<vk::PipelineLayout, 2> createMainPipelinesLayouts(
	vk::Device device,
	const PipelineDescriptorData& globalDescriptorData,
	const PipelineDescriptorData& materialDescriptorData,
	const PipelineDescriptorData& instanceRenderingDescriptorData
);

std::array<PipelineData, 1> createPostProcessingPipelines(
	vk::Device device,
	const RenderPassData& renderPasses,
	const PipelineDescriptorData& postProcessingDescriptorData,
	vk::ShaderModule vertexShader,
	vk::ShaderModule fragmentShader
);

void createNewVariant(
	MaterialPipeline& materialPipeline,
	const PipelineSpecializationConstants& specializationConstants,
	vk::Device device,
	vk::RenderPass renderPass,
	vk::ShaderModule vertexShader,
	vk::ShaderModule vertexShaderInstanced,
	vk::ShaderModule fragmentShader
);

bool hasPipeline(
	const MaterialPipeline& materialPipeline,
	const PipelineSpecializationConstants& specializationConstants
);

vk::Pipeline getRegularPipeline(
	const MaterialPipeline& materialPipeline,
	const PipelineSpecializationConstants& specializationConstants
);

vk::Pipeline getInstanceRenderingPipeline(
	const MaterialPipeline& materialPipeline,
	const PipelineSpecializationConstants& specializationConstants
);

void destroy(const MaterialPipeline& pipeline, vk::Device device);
void destroy(const PipelineData& pipelineData, vk::Device device);
void destroy(
	const PipelineDescriptorData& pipelineDescriptor, vk::Device device
);
}  // namespace graphics

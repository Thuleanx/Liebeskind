#pragma once

#include <cstdint>
#include <vulkan/vulkan.hpp>

#include "low_level_renderer/renderpass_data.h"

namespace graphics {

enum SamplerInclusionBits {
	eNone = 0,
	eAlbedo = 1,
	eNormal = 2,
	eDisplacement = 4,
	eEmission = 8,
};

using SamplerInclusion = uint32_t;

struct PipelineSpecializationConstants {
	SamplerInclusion samplerInclusion;

   public:
	bool operator==(const PipelineSpecializationConstants& other) const {
		return samplerInclusion == other.samplerInclusion;
	}

	bool operator<(const PipelineSpecializationConstants& other) const {
		return samplerInclusion < other.samplerInclusion;
	}
};

struct PipelineSpecializationConstantsHashFunction {
	inline size_t operator()(const PipelineSpecializationConstants& p) const {
		return p.samplerInclusion;
	}
};

struct PipelineTemplate {
	std::vector<vk::DynamicState> dynamicStates;
	vk::VertexInputBindingDescription vertexInputBinding;
	std::vector<vk::VertexInputAttributeDescription> vertexInputAttributes;
	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo;
	vk::PipelineViewportStateCreateInfo viewportStateInfo;
	vk::PipelineRasterizationStateCreateInfo rasterizerCreateInfo;
	vk::PipelineMultisampleStateCreateInfo multisamplingInfo;
	std::array<float, 4> colorBlendingConstants;
	vk::PipelineColorBlendAttachmentState colorBlendAttachment;
	vk::PipelineDepthStencilStateCreateInfo depthStencilState;

   public:
	static PipelineTemplate createDefault(const RenderPassData& renderPasses);
};

vk::Pipeline createVariant(
	const PipelineTemplate& pipelineTemplate,
	const PipelineSpecializationConstants& specializationConstants,
	vk::Device device,
	vk::RenderPass renderPass,
	vk::PipelineLayout pipelineLayout,
	vk::ShaderModule vertexShader,
	vk::ShaderModule fragmentShader
);

}  // namespace graphics

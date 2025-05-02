#pragma once

#include <cstdint>
#include <vulkan/vulkan.hpp>

#include "low_level_renderer/renderpass_data.h"

namespace graphics {

enum SamplerInclusionBits : uint32_t {
	eNone = 0,
	eAlbedo = 1,
	eNormal = 2,
	eDisplacement = 4,
	eEmission = 8,
};

enum class ParallaxMappingMode : uint32_t {
	eBasic = 0,
	eSteep = 1,
	eParallaxOcclusion = 2,
	eDeluxe = 3
};

using SamplerInclusion = uint32_t;

struct PipelineSpecializationConstants {
	SamplerInclusion samplerInclusion;
	ParallaxMappingMode parallaxMappingMode = ParallaxMappingMode::eDeluxe;

   public:
	bool operator==(const PipelineSpecializationConstants& other) const {
		return samplerInclusion == other.samplerInclusion &&
			   parallaxMappingMode == other.parallaxMappingMode;
	}

	bool operator<(const PipelineSpecializationConstants& other) const {
		if (samplerInclusion == other.samplerInclusion)
			return parallaxMappingMode < other.parallaxMappingMode;
		return samplerInclusion < other.samplerInclusion;
	}
};

constexpr std::array<vk::SpecializationMapEntry, 2> SPECIALIZATION_INFO = {
	vk::SpecializationMapEntry{0, 
		offsetof(PipelineSpecializationConstants, samplerInclusion),
        sizeof(SamplerInclusion)},
	vk::SpecializationMapEntry{
		1,
		offsetof(PipelineSpecializationConstants, parallaxMappingMode),
		sizeof(ParallaxMappingMode)
	}
};

struct PipelineSpecializationConstantsHashFunction {
	inline size_t operator()(const PipelineSpecializationConstants& p) const {
		return (p.samplerInclusion << 2) +
			   static_cast<size_t>(p.parallaxMappingMode);
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

std::vector<std::string> getGLSLDefinesFragment(
	const PipelineSpecializationConstants& variant
);

}  // namespace graphics

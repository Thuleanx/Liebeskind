#include "low_level_renderer/pipeline_template.h"

#include "core/logger/vulkan_ensures.h"
#include "low_level_renderer/vertex_buffer.h"

namespace graphics {

PipelineTemplate PipelineTemplate::createDefault(
	const RenderPassData& renderPasses
) {
	const std::vector<vk::DynamicState> dynamicStates = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor,
	};

	const auto vertexInputBinding = Vertex::getBindingDescription();
	const auto vertexInputAttributes = Vertex::getAttributeDescriptions();
	const std::vector<vk::VertexInputAttributeDescription>
		vertexInputAttributesVector = [&]() {
			std::vector<vk::VertexInputAttributeDescription> result;
			result.reserve(vertexInputAttributes.size());
			for (const vk::VertexInputAttributeDescription& inputAttribute :
				 vertexInputAttributes)
				result.push_back(inputAttribute);
			return result;
		}();

	const vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo(
		{},
		vk::PrimitiveTopology::eTriangleList,
		vk::False  // primitive restart
	);
	const vk::PipelineViewportStateCreateInfo viewportStateInfo(
		{}, 1, nullptr, 1, nullptr
	);

	const vk::PipelineRasterizationStateCreateInfo rasterizerCreateInfo(
		{},
		vk::False,	// depth clamp enable. only useful for shadow mapping
		vk::False,	// rasterizerDiscardEnable
		vk::PolygonMode::eFill,	 // fill polygon with fragments
		vk::CullModeFlagBits::eBack,
		vk::FrontFace::eCounterClockwise,
		vk::False,	// depth bias, probably useful for shadow mapping
		0.0f,
		0.0f,
		0.0f,
		1.0f  // line width
	);
	const vk::PipelineMultisampleStateCreateInfo multisamplingInfo(
		{},
		renderPasses.multisampleAntialiasingSampleCount,
		vk::True,
		0.2f,
		nullptr,
		vk::False,
		vk::False
	);
	const vk::PipelineColorBlendAttachmentState colorBlendAttachment(
		vk::True,  // enable blend
		vk::BlendFactor::eSrcAlpha,
		vk::BlendFactor::eOneMinusSrcAlpha,
		vk::BlendOp::eAdd,
		vk::BlendFactor::eOne,
		vk::BlendFactor::eZero,
		vk::BlendOp::eAdd,
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
			vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
	);
	const std::array<float, 4> colorBlendingConstants = {
		0.0f, 0.0f, 0.0f, 0.0f
	};
	const vk::PipelineDepthStencilStateCreateInfo depthStencilState(
		{},
		vk::False,	// enable depth test
		vk::False,	// enable depth write
		vk::CompareOp::eLess,
		vk::False,	// disable depth bounds test
		vk::False,	// disable stencil test
		{},			// stencil front op state
		{},			// stencil back op state
		{},			// min depth bound
		{}			// max depth bound
	);

	return {
		.dynamicStates = dynamicStates,
		.vertexInputBinding = vertexInputBinding,
		.vertexInputAttributes = vertexInputAttributesVector,
		.inputAssemblyStateInfo = inputAssemblyStateInfo,
		.viewportStateInfo = viewportStateInfo,
		.rasterizerCreateInfo = rasterizerCreateInfo,
		.multisamplingInfo = multisamplingInfo,
		.colorBlendingConstants = colorBlendingConstants,
		.colorBlendAttachment = colorBlendAttachment,
		.depthStencilState = depthStencilState,
	};
}

vk::Pipeline createVariant(
	const PipelineTemplate& pipelineTemplate,
	const PipelineSpecializationConstants& specializationConstants,
	vk::Device device,
	vk::RenderPass renderPass,
	vk::PipelineLayout pipelineLayout,
	vk::ShaderModule vertexShader,
	vk::ShaderModule fragmentShader
) {
	const vk::PipelineShaderStageCreateInfo vertexShaderStageInfo(
		{}, vk::ShaderStageFlagBits::eVertex, vertexShader, "main"
	);

	const std::array<vk::SpecializationMapEntry, 1> fragmentShaderConstants{
		vk::SpecializationMapEntry{0, 0, sizeof(SamplerInclusion)}
	};

	const vk::SpecializationInfo specializationInfo{
		fragmentShaderConstants.size(),
		fragmentShaderConstants.data(),
		sizeof(specializationConstants),
		&specializationConstants
	};

	const vk::PipelineShaderStageCreateInfo fragmentShaderStageInfo(
		{},
		vk::ShaderStageFlagBits::eFragment,
		fragmentShader,
		"main",
		&specializationInfo
	);

	const vk::PipelineShaderStageCreateInfo shaderStages[] = {
		vertexShaderStageInfo, fragmentShaderStageInfo
	};

	const vk::PipelineVertexInputStateCreateInfo vertexInputStateInfo(
		{},
		1,
		&pipelineTemplate.vertexInputBinding,
		static_cast<uint32_t>(pipelineTemplate.vertexInputAttributes.size()),
		pipelineTemplate.vertexInputAttributes.data()
	);

	const vk::PipelineDynamicStateCreateInfo dynamicStateInfo(
		{},
		static_cast<uint32_t>(pipelineTemplate.dynamicStates.size()),
		pipelineTemplate.dynamicStates.data()
	);

	const vk::PipelineColorBlendStateCreateInfo colorBlendingInfo(
		{},
		vk::False,
		vk::LogicOp::eCopy,
		1,
		&pipelineTemplate.colorBlendAttachment,
		pipelineTemplate.colorBlendingConstants
	);

	vk::GraphicsPipelineCreateInfo pipelineCreateInfo(
		{},
		2,
		shaderStages,
		&vertexInputStateInfo,
		&pipelineTemplate.inputAssemblyStateInfo,
		nullptr,  // no tesselation viewport
		&pipelineTemplate.viewportStateInfo,
		&pipelineTemplate.rasterizerCreateInfo,
		&pipelineTemplate.multisamplingInfo,
		&pipelineTemplate.depthStencilState,
		&colorBlendingInfo,
		&dynamicStateInfo,
		pipelineLayout,
		renderPass,
		0
	);
	const vk::ResultValue<vk::Pipeline> pipelineCreation =
		device.createGraphicsPipeline(nullptr, pipelineCreateInfo);
	VULKAN_ENSURE_SUCCESS(
		pipelineCreation.result, "Can't create graphics pipeline:"
	);
	return pipelineCreation.value;
}

std::vector<std::string> getGLSLDefinesFragment(
	const PipelineSpecializationConstants& variant
) {
    constexpr size_t NUM_OF_SAMPLER_OPTIONS = 4;
    
    constexpr std::array<SamplerInclusionBits, NUM_OF_SAMPLER_OPTIONS> samplerInclusionBit = {
        SamplerInclusionBits::eAlbedo,
        SamplerInclusionBits::eNormal,
        SamplerInclusionBits::eDisplacement,
        SamplerInclusionBits::eEmission,
    };
    const std::array<std::string, NUM_OF_SAMPLER_OPTIONS> glslDefineForSampleInclusion = {
        "HAS_ALBEDO",
        "HAS_NORMAL",
        "HAS_DISPLACEMENT",
        "HAS_EMISSION",
    };

    std::vector<std::string> glslDefines;

    for (size_t i = 0; i < NUM_OF_SAMPLER_OPTIONS; i++)
        if (variant.samplerInclusion & samplerInclusionBit[i])
            glslDefines.push_back(glslDefineForSampleInclusion[i]);

    return glslDefines;
}

}  // namespace graphics

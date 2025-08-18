#include "low_level_renderer/material_pipeline.h"

#include "core/logger/vulkan_ensures.h"
#include "low_level_renderer/config.h"
#include "low_level_renderer/materials.h"
#include "low_level_renderer/post_processing.h"
#include "low_level_renderer/shader_data.h"
#include "private/bloom.h"

namespace graphics {
MaterialPipeline MaterialPipeline::create(
	ShaderStorage& shaders,
	vk::Device device,
	vk::PhysicalDevice physicalDevice,
	const RenderPassData& renderPasses
) {
	PipelineDescriptorData globalDescriptorData;
	{  // create global descriptor information
		const vk::DescriptorSetLayoutBinding globalSceneDataBinding(
			0,	// binding
			vk::DescriptorType::eUniformBuffer,
			1,	// descriptor count
			vk::ShaderStageFlagBits::eVertex |
				vk::ShaderStageFlagBits::eFragment
		);
		const std::array<vk::DescriptorSetLayoutBinding, 1> globalBindings = {
			globalSceneDataBinding
		};
		const vk::ResultValue<vk::DescriptorSetLayout>
			globalDescriptorSetLayoutCreation =
				device.createDescriptorSetLayout(
					{{},
					 static_cast<uint32_t>(globalBindings.size()),
					 globalBindings.data()}
				);
		VULKAN_ENSURE_SUCCESS(
			globalDescriptorSetLayoutCreation.result,
			"Can't create global descriptor set layout"
		);
		std::vector<vk::DescriptorPoolSize> poolSizes = {
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1)
		};
		globalDescriptorData = {
			.setLayout = globalDescriptorSetLayoutCreation.value,
			.allocator = DescriptorAllocator::create(
				device, poolSizes, MAX_FRAMES_IN_FLIGHT
			)
		};
	}

	PipelineDescriptorData materialDescriptorData;
	{  // create material descriptor information
		const vk::ResultValue<vk::DescriptorSetLayout>
			materialDescriptorSetLayoutCreation =
				device.createDescriptorSetLayout(
					{{},
					 static_cast<uint32_t>(MATERIAL_BINDINGS.size()),
					 MATERIAL_BINDINGS.data()}
				);
		VULKAN_ENSURE_SUCCESS(
			materialDescriptorSetLayoutCreation.result,
			"Can't create material descriptor set layout"
		);
		materialDescriptorData = {
			.setLayout = materialDescriptorSetLayoutCreation.value,
			.allocator = DescriptorAllocator::create(
				device, MATERIAL_DESCRIPTOR_POOL_SIZES, MAX_FRAMES_IN_FLIGHT
			)
		};
	}

	PipelineDescriptorData instanceRenderingDescriptorData;
	{
		const vk::DescriptorSetLayoutBinding instancedRenderingBinding(
			0,	// binding
			vk::DescriptorType::eStorageBuffer,
			1,	// descriptor count
			vk::ShaderStageFlagBits::eVertex
		);
		const std::array<vk::DescriptorSetLayoutBinding, 1>
			instancedRenderingBindings = {instancedRenderingBinding};
		const vk::ResultValue<vk::DescriptorSetLayout>
			instancedRenderingDescriptorSetLayoutCreation =
				device.createDescriptorSetLayout(
					{{},
					 static_cast<uint32_t>(instancedRenderingBindings.size()),
					 instancedRenderingBindings.data()}
				);
		VULKAN_ENSURE_SUCCESS(
			instancedRenderingDescriptorSetLayoutCreation.result,
			"Can't create instancedRendering descriptor set layout"
		);

		std::vector<vk::DescriptorPoolSize> poolSizes = {
			vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1)
		};
		instanceRenderingDescriptorData = {
			.setLayout = instancedRenderingDescriptorSetLayoutCreation.value,
			.allocator = DescriptorAllocator::create(
				device, poolSizes, MAX_FRAMES_IN_FLIGHT
			)
		};
	}

	PipelineDescriptorData postProcessingDescriptorData;
	{
		const vk::ResultValue<vk::DescriptorSetLayout>
			postProcessingDescriptorSetLayoutCreation =
				device.createDescriptorSetLayout(
					{{},
					 static_cast<uint32_t>(POST_PROCESSING_BINDINGS.size()),
					 POST_PROCESSING_BINDINGS.data()}
				);
		VULKAN_ENSURE_SUCCESS(
			postProcessingDescriptorSetLayoutCreation.result,
			"Can't create postProcessing descriptor set layout"
		);
		postProcessingDescriptorData = {
			.setLayout = postProcessingDescriptorSetLayoutCreation.value,
			.allocator = DescriptorAllocator::create(
				device,
				POST_PROCESSING_DESCRIPTOR_POOL_SIZES,
				MAX_FRAMES_IN_FLIGHT
			)
		};
	}

	const PipelineTemplate pipelineTemplate =
		PipelineTemplate::createDefault(renderPasses);
	const auto [regularPipelineLayout, instanceRenderingPipelineLayout] =
		createMainPipelinesLayouts(
			device,
			globalDescriptorData,
			materialDescriptorData,
			instanceRenderingDescriptorData
		);

	const auto [postProcessingPipeline] = createPostProcessingPipelines(
		shaders, device, renderPasses, postProcessingDescriptorData
	);

	const BloomPipeline bloomPipeline =
		createBloomPipeline(shaders, device, physicalDevice, renderPasses);

	return {
		.pipelineTemplate = pipelineTemplate,
		.regularPipelineVariants = {},
		.instanceRenderingPipelineVariants = {},
		.regularPipelineLayout = regularPipelineLayout,
		.instanceRenderingPipelineLayout = instanceRenderingPipelineLayout,
		.postProcessingPipeline = postProcessingPipeline,
		.globalDescriptor = globalDescriptorData,
		.instanceRenderingDescriptor = instanceRenderingDescriptorData,
		.materialDescriptor = materialDescriptorData,
		.postProcessingDescriptor = postProcessingDescriptorData,
		.bloomPipeline = bloomPipeline,
	};
}

std::array<vk::PipelineLayout, 2> createMainPipelinesLayouts(
	vk::Device device,
	const PipelineDescriptorData& globalDescriptorData,
	const PipelineDescriptorData& materialDescriptorData,
	const PipelineDescriptorData& instanceRenderingDescriptorData
) {
	const std::vector<vk::DescriptorSetLayout> instanceRenderingSetLayouts = {
		globalDescriptorData.setLayout,
		materialDescriptorData.setLayout,
		instanceRenderingDescriptorData.setLayout,
	};

	const std::vector<vk::DescriptorSetLayout> regularSetLayouts = {
		globalDescriptorData.setLayout,
		materialDescriptorData.setLayout,
	};

	const vk::PushConstantRange pushConstantRange(
		vk::ShaderStageFlagBits::eVertex, 0, sizeof(GPUPushConstants)
	);

	const vk::PipelineLayout regularPipelineLayout = [&]() {
		const vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
			{},
			regularSetLayouts.size(),
			regularSetLayouts.data(),
			1,
			&pushConstantRange
		);
		const vk::ResultValue<vk::PipelineLayout> pipelineLayoutCreation =
			device.createPipelineLayout(pipelineLayoutInfo);
		VULKAN_ENSURE_SUCCESS(
			pipelineLayoutCreation.result, "Can't create pipeline layout:"
		);
		return pipelineLayoutCreation.value;
	}();

	const vk::PipelineLayout instanceRenderingPipelineLayout = [&]() {
		const vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
			{},
			instanceRenderingSetLayouts.size(),
			instanceRenderingSetLayouts.data(),
			0,
			nullptr
		);
		const vk::ResultValue<vk::PipelineLayout> pipelineLayoutCreation =
			device.createPipelineLayout(pipelineLayoutInfo);
		VULKAN_ENSURE_SUCCESS(
			pipelineLayoutCreation.result, "Can't create pipeline layout:"
		);
		return pipelineLayoutCreation.value;
	}();

	return {regularPipelineLayout, instanceRenderingPipelineLayout};
}

std::array<PipelineData, 1> createPostProcessingPipelines(
	ShaderStorage& shaders,
	vk::Device device,
	const RenderPassData& renderPasses,
	const PipelineDescriptorData& postProcessingDescriptorData
) {
	const std::vector<vk::DescriptorSetLayout> setLayouts = {
		postProcessingDescriptorData.setLayout,
	};

	const ShaderID vertexShaderID = loadShaderFromFile(
		shaders, device, "shaders/post_processing.vert.glsl.spv"
	);
	const ShaderID fragmentShaderID = loadShaderFromFile(
		shaders, device, "shaders/post_processing.frag.glsl.spv"
	);

	const vk::PipelineShaderStageCreateInfo vertexShaderStageInfo(
		{},
		vk::ShaderStageFlagBits::eVertex,
		getModule(shaders, vertexShaderID),
		"main"
	);
	const vk::PipelineShaderStageCreateInfo fragmentShaderStageInfo(
		{},
		vk::ShaderStageFlagBits::eFragment,
		getModule(shaders, fragmentShaderID),
		"main"
	);

	const std::vector<vk::DynamicState> dynamicStates = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor,
	};
	const vk::PipelineDynamicStateCreateInfo dynamicStateInfo(
		{}, static_cast<uint32_t>(dynamicStates.size()), dynamicStates.data()
	);

	const vk::PipelineVertexInputStateCreateInfo vertexInputStateInfo(
		{}, 0, nullptr, 0, nullptr
	);
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
		vk::PolygonMode::eFill,		  // fill polygon with fragments
		vk::CullModeFlagBits::eNone,  // we need this for our vertex shader to
									  // work
		vk::FrontFace::eCounterClockwise,
		vk::False,	// depth bias, probably useful for shadow mapping
		0.0f,
		0.0f,
		0.0f,
		1.0f  // line width
	);
	const vk::PipelineMultisampleStateCreateInfo multisamplingInfo(
		{},
		vk::SampleCountFlagBits::e1,
		vk::False,
		1.0f,
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
	const vk::PipelineColorBlendStateCreateInfo colorBlendingInfo(
		{},
		vk::False,
		vk::LogicOp::eCopy,
		1,
		&colorBlendAttachment,
		colorBlendingConstants
	);
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

	const vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
		{}, setLayouts.size(), setLayouts.data(), 0, nullptr
	);
	const vk::ResultValue<vk::PipelineLayout> pipelineLayoutCreation =
		device.createPipelineLayout(pipelineLayoutInfo);
	VULKAN_ENSURE_SUCCESS(
		pipelineLayoutCreation.result, "Can't create pipeline layout:"
	);
	const vk::PipelineShaderStageCreateInfo shaderStages[] = {
		vertexShaderStageInfo, fragmentShaderStageInfo
	};
	vk::GraphicsPipelineCreateInfo pipelineCreateInfo(
		{},
		2,
		shaderStages,
		&vertexInputStateInfo,
		&inputAssemblyStateInfo,
		nullptr,  // no tesselation viewport
		&viewportStateInfo,
		&rasterizerCreateInfo,
		&multisamplingInfo,
		&depthStencilState,
		&colorBlendingInfo,
		&dynamicStateInfo,
		pipelineLayoutCreation.value,
		renderPasses.postProcessingPass,
		0
	);
	const vk::ResultValue<vk::Pipeline> pipelineCreation =
		device.createGraphicsPipeline(nullptr, pipelineCreateInfo);
	VULKAN_ENSURE_SUCCESS(
		pipelineCreation.result, "Can't create graphics pipeline:"
	);

	return {PipelineData{
		.pipeline = pipelineCreation.value,
		.layout = pipelineLayoutCreation.value
	}};
}

void createNewVariant(
	MaterialPipeline& materialPipeline,
	const PipelineSpecializationConstants& specializationConstants,
	vk::Device device,
	vk::RenderPass renderPass,
	vk::ShaderModule vertexShader,
	vk::ShaderModule vertexShaderInstanced,
	vk::ShaderModule fragmentShader
) {
	const vk::Pipeline regularPipeline = createVariant(
		materialPipeline.pipelineTemplate,
		specializationConstants,
		device,
		renderPass,
		materialPipeline.regularPipelineLayout,
		vertexShader,
		fragmentShader
	);

	const vk::Pipeline instancedPipeline = createVariant(
		materialPipeline.pipelineTemplate,
		specializationConstants,
		device,
		renderPass,
		materialPipeline.instanceRenderingPipelineLayout,
		vertexShaderInstanced,
		fragmentShader
	);

	materialPipeline.regularPipelineVariants[specializationConstants] =
		regularPipeline;
	materialPipeline
		.instanceRenderingPipelineVariants[specializationConstants] =
		instancedPipeline;
}

bool hasPipeline(
	const MaterialPipeline& materialPipeline,
	const PipelineSpecializationConstants& specializationConstants
) {
	return materialPipeline.regularPipelineVariants.contains(
		specializationConstants
	);
}

vk::Pipeline getRegularPipeline(
	const MaterialPipeline& materialPipeline,
	const PipelineSpecializationConstants& specializationConstants
) {
	ASSERT(
		materialPipeline.regularPipelineVariants.contains(
			specializationConstants
		),
		"Material pipeline has no entry for specialization constant: "
			<< specializationConstants.samplerInclusion
	);
	return materialPipeline.regularPipelineVariants.at(specializationConstants);
}

vk::Pipeline getInstanceRenderingPipeline(
	const MaterialPipeline& materialPipeline,
	const PipelineSpecializationConstants& specializationConstants
) {
	ASSERT(
		materialPipeline.instanceRenderingPipelineVariants.contains(
			specializationConstants
		),
		"Material pipeline has no entry for specialization constant: "
			<< specializationConstants.samplerInclusion
	);
	return materialPipeline.instanceRenderingPipelineVariants.at(
		specializationConstants
	);
}

void destroy(const MaterialPipeline& pipeline, vk::Device device) {
	destroy(pipeline.globalDescriptor, device);
	destroy(pipeline.instanceRenderingDescriptor, device);
	destroy(pipeline.materialDescriptor, device);
	for (const auto& [constants, pipeline] : pipeline.regularPipelineVariants)
		device.destroyPipeline(pipeline);
	for (const auto& [constants, pipeline] :
		 pipeline.instanceRenderingPipelineVariants)
		device.destroyPipeline(pipeline);
	device.destroyPipelineLayout(pipeline.regularPipelineLayout);
	device.destroyPipelineLayout(pipeline.instanceRenderingPipelineLayout);
	destroy(pipeline.postProcessingDescriptor, device);
	destroy(pipeline.postProcessingPipeline, device);
	destroy(pipeline.bloomPipeline, device);
}

void destroy(const PipelineData& pipelineData, vk::Device device) {
	device.destroy(pipelineData.pipeline);
	device.destroy(pipelineData.layout);
}

void destroy(
	const PipelineDescriptorData& pipelineDescriptor, vk::Device device
) {
	pipelineDescriptor.allocator.destroyBy(device);
	device.destroy(pipelineDescriptor.setLayout);
}
}  // namespace graphics

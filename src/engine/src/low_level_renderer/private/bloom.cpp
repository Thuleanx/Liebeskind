#include "bloom.h"

#include "core/logger/assert.h"
#include "core/logger/vulkan_ensures.h"
#include "descriptor.h"
#include "image.h"
#include "low_level_renderer/descriptor_write_buffer.h"
#include "low_level_renderer/shaders.h"
#include "pipeline.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include "imgui.h"
#pragma GCC diagnostic pop

namespace {
struct CombineSpecializationConstants {
	int numLayers;
};

struct UpsampleSpecializationConstants {
	float currentMipScale;
};

struct DownsampleSpecializationConstants {
	float currentMipScale;
	bool shouldUseKarisAverage;
};

enum class BloomDescriptorSetBindingPoint {
	eLayer = 0,
	eShared = 1,
};

};	// namespace

namespace graphics {
void recordBloomRenderpass(
	Module& module, RenderSubmission& renderSubmission, vk::CommandBuffer buffer
) {

	ASSERT(module.device.swapchain.has_value(), "Cannot record if the swapchain is empty");
	ASSERT(module.device.bloom.swapchainObject.has_value(), "Cannot record if the swapchain is empty");

	const BloomGraphicsObjects::SwapchainObject& bloomSwapchainObject =
		module.device.bloom.swapchainObject.value();

	const std::array<vk::ImageSubresourceRange, 1> subresources = {
		vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, NUM_BLOOM_MIPS, 0, 1)
	};

	for (size_t pass = 0; pass < NUM_BLOOM_PASSES; pass++) {
		const bool isCombinePass = pass == NUM_BLOOM_PASSES - 1;
		const size_t mip = getBloomRenderMip(pass);

		// Technically we only have to bloom the main window,
		// but just so that the bloom is consistent we shall bloom the entire
		// image
		const vk::Viewport viewport(
			0,
			0,
			module.device.swapchain->extent.width / (1u << mip),
			module.device.swapchain->extent.height / (1u << mip),
			0.0f,
			1.0f
		);
		buffer.setViewport(0, 1, &viewport);

		const vk::Rect2D renderRect(
			{0, 0},
			{
				module.device.swapchain->extent.width / (1u << mip),
				module.device.swapchain->extent.height / (1u << mip),
			}
		);
		buffer.setScissor(0, 1, &renderRect);
		const vk::RenderPass renderPass = isCombinePass				? module.device.bloom.renderPasses.combine
										  : pass < NUM_BLOOM_LAYERS ? module.device.bloom.renderPasses.downsample
																	: module.device.bloom.renderPasses.upsample;
		const vk::PipelineLayout pipelineLayout = isCombinePass ? module.device.bloom.pipelineLayouts.combine
												  : pass < NUM_BLOOM_LAYERS
													  ? module.device.bloom.pipelineLayouts.downsample
													  : module.device.bloom.pipelineLayouts.upsample;

		const std::array<vk::ClearValue, 1> clearColors = {
			vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f),
		};
		const vk::RenderPassBeginInfo renderPassInfo(
			renderPass, bloomSwapchainObject.framebuffers[pass], renderRect, clearColors.size(), clearColors.data()
		);
		buffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
		buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, module.device.bloom.pipelines[pass]);
		buffer.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics,
			pipelineLayout,
			static_cast<int>(BloomDescriptorSetBindingPoint::eShared),
			1,
			&module.device.bloom.descriptors.shared,
			0,
			nullptr
		);

		if (isCombinePass) {
			buffer.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				pipelineLayout,
				static_cast<int>(BloomDescriptorSetBindingPoint::eLayer),
				1,
				&bloomSwapchainObject.descriptors.combine,
				0,
				nullptr
			);
		} else {
			const bool isDownsamplePass = pass < NUM_BLOOM_LAYERS;
			if (isDownsamplePass) {
				buffer.bindDescriptorSets(
					vk::PipelineBindPoint::eGraphics,
					pipelineLayout,
					static_cast<int>(BloomDescriptorSetBindingPoint::eLayer),
					1,
					&bloomSwapchainObject.descriptors.downsample[pass],
					0,
					nullptr
				);
			} else {
				ASSERT(pass >= NUM_BLOOM_LAYERS, "Pass " << pass << " should be ");
				buffer.bindDescriptorSets(
					vk::PipelineBindPoint::eGraphics,
					pipelineLayout,
					static_cast<int>(BloomDescriptorSetBindingPoint::eLayer),
					1,
					&bloomSwapchainObject.descriptors.upsample[pass - NUM_BLOOM_LAYERS],
					0,
					nullptr
				);
			}
		}

		buffer.draw(6, 1, 0, 0);
		buffer.endRenderPass();
	}
}

BloomGraphicsObjects createBloomObjects(
	vk::Device device, vk::PhysicalDevice physicalDevice, ShaderStorage& shaders, vk::Format colorFormat
) {
	ASSERT(
		NUM_BLOOM_PASSES > 0,
		"Number of bloom passes cannot be 0. " << "If not using bloom, then don't call this function."
	);

	const auto createRenderPass = [](vk::Device device, vk::ImageLayout attachmentInitialLayout, vk::Format colorFormat
								  ) {
		const std::array<vk::AttachmentDescription, 1> attachments = {
			vk::AttachmentDescription(
				{},
				colorFormat,
				vk::SampleCountFlagBits::e1,
				vk::AttachmentLoadOp::eDontCare,
				vk::AttachmentStoreOp::eStore,
				vk::AttachmentLoadOp::eDontCare,
				vk::AttachmentStoreOp::eDontCare,
				attachmentInitialLayout,
				vk::ImageLayout::eShaderReadOnlyOptimal
			),
		};
		constexpr std::array<vk::AttachmentReference, 1> outputAttachments = {
			vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal)
		};
		const vk::SubpassDescription mainSubpassDescriptions(
			{},
			vk::PipelineBindPoint::eGraphics,
			// input refs
			0,
			nullptr,
			// color refs
			outputAttachments.size(),
			outputAttachments.data(),
			nullptr,
			nullptr
		);
		const vk::SubpassDependency subpassDependency(
			vk::SubpassExternal,
			0,
			vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
			vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
			{},
			vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite
		);
		const vk::RenderPassCreateInfo renderPassInfo(
			{}, attachments.size(), attachments.data(), 1, &mainSubpassDescriptions, 1, &subpassDependency
		);
		const vk::ResultValue<vk::RenderPass> renderPassCreation = device.createRenderPass(renderPassInfo);
		VULKAN_ENSURE_SUCCESS(renderPassCreation.result, "Can't create bloom renderpass:");
		return renderPassCreation.value;
	};
	// TODO: optimize it to just one renderpass should multiple not be needed
	const BloomGraphicsObjects::RenderPasses renderPasses = {
		.downsample = createRenderPass(device, vk::ImageLayout::eUndefined, colorFormat),
		.upsample = createRenderPass(device, vk::ImageLayout::eUndefined, colorFormat),
		.combine = createRenderPass(device, vk::ImageLayout::eUndefined, colorFormat)
	};

	constexpr std::array<vk::DescriptorSetLayoutBinding, 3> COMBINE_LAYER_LAYOUT = {
		vk::DescriptorSetLayoutBinding{
			0,	// binding
			vk::DescriptorType::eCombinedImageSampler,
			1,	// descriptor count
			vk::ShaderStageFlagBits::eFragment
		},
		vk::DescriptorSetLayoutBinding{
			1,	// binding
			vk::DescriptorType::eCombinedImageSampler,
			1,	// descriptor count
			vk::ShaderStageFlagBits::eFragment
		},
		vk::DescriptorSetLayoutBinding{
			2,	// binding
			vk::DescriptorType::eUniformBuffer,
			1,	// descriptor count
			vk::ShaderStageFlagBits::eFragment
		}
	};
	constexpr std::array<vk::DescriptorSetLayoutBinding, 1> DOWNSAMPLE_LAYER_LAYOUT = {vk::DescriptorSetLayoutBinding{
		0,	// binding
		vk::DescriptorType::eCombinedImageSampler,
		1,	// descriptor count
		vk::ShaderStageFlagBits::eFragment
	}};
	constexpr std::array<vk::DescriptorSetLayoutBinding, 3> UPSAMPLE_LAYER_LAYOUT = {
		vk::DescriptorSetLayoutBinding{
			0,	// binding
			vk::DescriptorType::eCombinedImageSampler,
			1,	// descriptor count
			vk::ShaderStageFlagBits::eFragment
		},
		vk::DescriptorSetLayoutBinding{
			1,	// binding
			vk::DescriptorType::eCombinedImageSampler,
			1,	// descriptor count
			vk::ShaderStageFlagBits::eFragment
		},
		vk::DescriptorSetLayoutBinding{
			2,	// binding
			vk::DescriptorType::eUniformBuffer,
			1,	// descriptor count
			vk::ShaderStageFlagBits::eFragment
		}
	};
	constexpr std::array<vk::DescriptorSetLayoutBinding, 1> SHARED_LAYOUT = {vk::DescriptorSetLayoutBinding{
		0,	// binding
		vk::DescriptorType::eUniformBuffer,
		1,	// descriptor count
		vk::ShaderStageFlagBits::eFragment
	}};

	const BloomGraphicsObjects::LayerSetLayouts uniformLayouts = {
		.downsample = createDescriptorLayout(device, DOWNSAMPLE_LAYER_LAYOUT),
		.upsample = createDescriptorLayout(device, UPSAMPLE_LAYER_LAYOUT),
		.combine = createDescriptorLayout(device, COMBINE_LAYER_LAYOUT),
		.shared = createDescriptorLayout(device, SHARED_LAYOUT),
	};

	const std::array<vk::DescriptorSetLayout, 2> downsamplePipelineLayoutSets = {
		uniformLayouts.downsample, uniformLayouts.shared
	};
	const std::array<vk::DescriptorSetLayout, 2> upsamplePipelineLayoutSets = {
		uniformLayouts.upsample, uniformLayouts.shared
	};
	const std::array<vk::DescriptorSetLayout, 2> combinePipelineLayoutSets = {
		uniformLayouts.combine, uniformLayouts.shared
	};
	const BloomGraphicsObjects::PipelineLayouts pipelineLayouts = {
		.downsample = createPipelineLayout(device, downsamplePipelineLayoutSets, {}),
		.upsample = createPipelineLayout(device, upsamplePipelineLayoutSets, {}),
		.combine = createPipelineLayout(device, combinePipelineLayoutSets, {}),
	};

	const std::vector<vk::DynamicState> dynamicStates = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor,
	};
	const vk::PipelineDynamicStateCreateInfo dynamicStateInfo(
		{}, static_cast<uint32_t>(dynamicStates.size()), dynamicStates.data()
	);
	const vk::PipelineVertexInputStateCreateInfo vertexInputStateInfo({}, 0, nullptr, 0, nullptr);
	const vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo(
		{},
		vk::PrimitiveTopology::eTriangleList,
		vk::False  // primitive restart
	);
	const vk::PipelineViewportStateCreateInfo viewportStateInfo({}, 1, nullptr, 1, nullptr);
	const vk::PipelineRasterizationStateCreateInfo rasterizerCreateInfo(
		{},
		vk::False,					  // depth clamp enable. only useful for shadow mapping
		vk::False,					  // rasterizerDiscardEnable
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
		{}, vk::SampleCountFlagBits::e1, vk::False, 1.0f, nullptr, vk::False, vk::False
	);
	const std::array<float, 4> colorBlendingConstants = {0.0f, 0.0f, 0.0f, 0.0f};
	const std::array<vk::PipelineColorBlendAttachmentState, 1> colorBlendStates = {
		vk::PipelineColorBlendAttachmentState(
			vk::True,  // disable blend
			vk::BlendFactor::eOne,
			vk::BlendFactor::eZero,
			vk::BlendOp::eAdd,
			vk::BlendFactor::eOne,
			vk::BlendFactor::eOne,
			vk::BlendOp::eMax,
			vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
				vk::ColorComponentFlagBits::eA
		)
	};
	const vk::PipelineColorBlendStateCreateInfo colorBlendingInfo(
		{}, vk::False, vk::LogicOp::eCopy, colorBlendStates.size(), colorBlendStates.data(), colorBlendingConstants
	);
	const vk::PipelineDepthStencilStateCreateInfo depthStencilState(
		{},
		vk::False,	// disable depth test
		vk::False,	// disable depth write
		vk::CompareOp::eLess,
		vk::False,	// disable depth bounds test
		vk::False,	// disable stencil test
		{},			// stencil front op state
		{},			// stencil back op state
		{},			// min depth bound
		{}			// max depth bound
	);

	const ShaderID vertexShaderID = loadShaderFromFile(shaders, device, "shaders/entire_screen.vert.glsl.spv");
	const ShaderID downsampleShaderID = loadShaderFromFile(shaders, device, "shaders/bloom_downsample.frag.glsl.spv");
	const ShaderID upsampleShaderID = loadShaderFromFile(shaders, device, "shaders/bloom_upsample.frag.glsl.spv");
	const ShaderID combineShaderID = loadShaderFromFile(shaders, device, "shaders/bloom_combine.frag.glsl.spv");

	const vk::PipelineShaderStageCreateInfo vertexShaderInfo(
		{}, vk::ShaderStageFlagBits::eVertex, getModule(shaders, vertexShaderID), "main", nullptr
	);

	constexpr std::array<vk::SpecializationMapEntry, 2> SPECIALIZATION_INFO_DOWNSAMPLE_ENTRY = {
		vk::SpecializationMapEntry{0, offsetof(DownsampleSpecializationConstants, currentMipScale), sizeof(float)},
		vk::SpecializationMapEntry{1, offsetof(DownsampleSpecializationConstants, shouldUseKarisAverage), 4}
	};
	constexpr std::array<vk::SpecializationMapEntry, 1> SPECIALIZATION_INFO_UPSAMPLE_ENTRY = {
		vk::SpecializationMapEntry{0, offsetof(UpsampleSpecializationConstants, currentMipScale), sizeof(float)},
	};
	constexpr std::array<vk::SpecializationMapEntry, 1> SPECIALIZATION_INFO_COMBINE_ENTRY = {
		vk::SpecializationMapEntry{0, offsetof(CombineSpecializationConstants, numLayers), sizeof(int)},
	};

	std::array<vk::Pipeline, NUM_BLOOM_PASSES> pipelines;
	std::array<DownsampleSpecializationConstants, NUM_BLOOM_LAYERS> downsampleConstants;
	std::array<UpsampleSpecializationConstants, NUM_BLOOM_LAYERS - 1> upsampleConstants;
	std::array<vk::SpecializationInfo, NUM_BLOOM_PASSES - 1> specializationInfo;

	for (uint32_t subpass = 0; subpass < NUM_BLOOM_LAYERS; subpass++) {
		const uint32_t divisions = getBloomRenderMip(subpass);
		downsampleConstants[subpass] = {
			.currentMipScale = static_cast<float>(1u << divisions), .shouldUseKarisAverage = (subpass == 0)
		};
		specializationInfo[subpass] = {
			SPECIALIZATION_INFO_DOWNSAMPLE_ENTRY.size(),
			SPECIALIZATION_INFO_DOWNSAMPLE_ENTRY.data(),
			sizeof(downsampleConstants[subpass]),
			&downsampleConstants[subpass]
		};
	}

	for (uint32_t subpass = NUM_BLOOM_LAYERS; subpass < NUM_BLOOM_PASSES - 1; subpass++) {
		const uint32_t divisions = getBloomRenderMip(subpass);
		upsampleConstants[subpass - NUM_BLOOM_LAYERS] = {
			.currentMipScale = static_cast<float>(1u << divisions),
		};
		specializationInfo[subpass] = {
			SPECIALIZATION_INFO_UPSAMPLE_ENTRY.size(),
			SPECIALIZATION_INFO_UPSAMPLE_ENTRY.data(),
			sizeof(upsampleConstants[subpass - NUM_BLOOM_LAYERS]),
			&upsampleConstants[subpass - NUM_BLOOM_LAYERS]
		};
	}

	vk::GraphicsPipelineCreateInfo pipelineCreateInfo(
		{},
		0,
		nullptr,
		&vertexInputStateInfo,
		&inputAssemblyStateInfo,
		nullptr,  // no tesselation viewport
		&viewportStateInfo,
		&rasterizerCreateInfo,
		&multisamplingInfo,
		&depthStencilState,
		&colorBlendingInfo,
		&dynamicStateInfo,
		{},
		{},
		0  // subpass = 0 since we use one subpass per renderpass
	);

	for (uint32_t subpass = 0; subpass < NUM_BLOOM_PASSES - 1; subpass++) {
		const bool isDownsamplePass = subpass < NUM_BLOOM_LAYERS;

		const std::string kernelName = "main";
		const vk::PipelineShaderStageCreateInfo fragmentShaderInfo(
			{},
			vk::ShaderStageFlagBits::eFragment,
			getModule(shaders, isDownsamplePass ? downsampleShaderID : upsampleShaderID),
			kernelName.c_str(),
			&specializationInfo[subpass]
		);

		const std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {vertexShaderInfo, fragmentShaderInfo};

		pipelineCreateInfo.setStageCount(shaderStages.size());
		pipelineCreateInfo.setStages(shaderStages);
		pipelineCreateInfo.setLayout(isDownsamplePass ? pipelineLayouts.downsample : pipelineLayouts.upsample);
		pipelineCreateInfo.setRenderPass(isDownsamplePass ? renderPasses.downsample : renderPasses.upsample);

		const vk::ResultValue<vk::Pipeline> pipelineCreation =
			device.createGraphicsPipeline(nullptr, pipelineCreateInfo);
		VULKAN_ENSURE_SUCCESS(pipelineCreation.result, "Can't create bloom pipeline:");
		pipelines[subpass] = pipelineCreation.value;
	}

	{
		const std::string kernelName = "main";

		const CombineSpecializationConstants specializationConstant = {.numLayers = NUM_BLOOM_LAYERS};
		const vk::SpecializationInfo specializationInfo = {
			SPECIALIZATION_INFO_COMBINE_ENTRY.size(),
			SPECIALIZATION_INFO_COMBINE_ENTRY.data(),
			sizeof(specializationConstant),
			&specializationConstant
		};

		const vk::PipelineShaderStageCreateInfo fragmentShaderInfo(
			{},
			vk::ShaderStageFlagBits::eFragment,
			getModule(shaders, combineShaderID),
			kernelName.c_str(),
			&specializationInfo
		);

		const std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {vertexShaderInfo, fragmentShaderInfo};

		pipelineCreateInfo.setStageCount(shaderStages.size());
		pipelineCreateInfo.setStages(shaderStages);
		pipelineCreateInfo.setLayout(pipelineLayouts.combine);
		pipelineCreateInfo.setRenderPass(renderPasses.combine);

		const vk::ResultValue<vk::Pipeline> pipelineCreation =
			device.createGraphicsPipeline(nullptr, pipelineCreateInfo);
		VULKAN_ENSURE_SUCCESS(pipelineCreation.result, "Can't create bloom pipeline:");
		pipelines[NUM_BLOOM_PASSES - 1] = pipelineCreation.value;
	}

	constexpr std::array<vk::DescriptorPoolSize, 1> DOWNSAMPLE_POOL_SIZES = {
		vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1),
	};
	constexpr std::array<vk::DescriptorPoolSize, 2> UPSAMPLE_POOL_SIZES = {
		vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 2),
		vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1),
	};
	constexpr std::array<vk::DescriptorPoolSize, 2> COMBINE_POOL_SIZES = {
		vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 2),
		vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1),
	};
	constexpr std::array<vk::DescriptorPoolSize, 1> SHARED_POOL_SIZES = {
		vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1),
	};

	const BloomGraphicsObjects::DescriptorPools pools = {
		.layer_downsample = DescriptorAllocator::create(device, DOWNSAMPLE_POOL_SIZES, MAX_FRAMES_IN_FLIGHT),
		.layer_upsample = DescriptorAllocator::create(device, UPSAMPLE_POOL_SIZES, MAX_FRAMES_IN_FLIGHT),
		.layer_combine = DescriptorAllocator::create(device, COMBINE_POOL_SIZES, MAX_FRAMES_IN_FLIGHT),
		.shared = createDescriptorPool(device, 1, SHARED_POOL_SIZES)
	};

	const std::vector<vk::DescriptorSet> sharedDescriptors =
		createDescriptorSets(device, pools.shared, uniformLayouts.shared, 1);

	const BloomGraphicsObjects::Descriptors descriptors = {.shared = sharedDescriptors.front()};

	// TODO: optimize this write buffer. It's using way too much memory.
	// Write buffers in general is good and perhaps we should have a global one
	DescriptorWriteBuffer writeBuffer;

	const BloomGraphicsObjects::DataBuffers buffers{
		.upsample = UniformBuffer<BloomUpsampleBuffer>::create(device, physicalDevice, 1),
		.combine = UniformBuffer<BloomCombineBuffer>::create(device, physicalDevice, 1),
		.shared = UniformBuffer<BloomSharedBuffer>::create(device, physicalDevice, 1),
	};

	const BloomGraphicsObjects::Config config;

	buffers.combine.update(BloomCombineBuffer{
		.intensity = config.intensity,
		.blurRadius = config.blurRadius,
	});
	buffers.upsample.update(BloomUpsampleBuffer{
		.blurRadius = config.blurRadius,
	});

	buffers.shared.bind(writeBuffer, descriptors.shared, 0);
	writeBuffer.flush(device);

	return BloomGraphicsObjects{
		.config = config,
		.renderPasses = renderPasses,
		.uniformLayouts = uniformLayouts,
		.pipelineLayouts = pipelineLayouts,
		.pools = pools,
		.descriptors = descriptors,
		.buffers = buffers,
		.pipelines = pipelines,
		.swapchainObject = {}
	};
}

void updateConfigOnGPU(const BloomGraphicsObjects& obj) {
	obj.buffers.combine.update(BloomCombineBuffer{
		.intensity = obj.config.intensity,
		.blurRadius = obj.config.blurRadius,
	});
	obj.buffers.upsample.update(BloomUpsampleBuffer{
		.blurRadius = obj.config.blurRadius,
	});
}

BloomGraphicsObjects::SwapchainObject createBloomSwapchainObject(
	BloomSwapchainObjectCreateInfo createInfo
) {
	DescriptorWriteBuffer writeBuffer;
    const vk::Format imageFormat = createInfo.colorBuffer.format;

    constexpr size_t NUM_BUFFERS = BloomGraphicsObjects::SwapchainObject::NUM_BUFFERS;

    std::array<vk::Image, NUM_BUFFERS> images;
    std::array<vk::DeviceMemory, NUM_BUFFERS> memory;
    std::array<std::array<vk::ImageView, NUM_BLOOM_MIPS>, NUM_BUFFERS> colorViews;
    for (size_t image_index = 0; image_index < 2; image_index++) {
        std::tie(images[image_index], memory[image_index]) = Image::create(
            Image::CreateInfo {
                device: createInfo.device,
                physicalDevice: createInfo.physicalDevice,
                size: vk::Extent3D(createInfo.swapchainExtent.width, createInfo.swapchainExtent.height, 1),
                format: imageFormat,
                usage: vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
                mipLevels: NUM_BLOOM_MIPS
            }
        );

        for (size_t i = 0; i < NUM_BLOOM_MIPS; i++)
            colorViews[image_index][i] = Image::createImageView(
                createInfo.device, images[image_index], vk::ImageViewType::e2D, imageFormat, vk::ImageAspectFlagBits::eColor, i, 1
            );
    }

    std::array<vk::Framebuffer, NUM_BLOOM_PASSES> framebuffers;
    for (size_t pass = 0; pass < NUM_BLOOM_PASSES; pass++) {
        const size_t mip = getBloomRenderMip(pass);
        ASSERT(mip >= 0 && mip < NUM_BLOOM_MIPS, "Mip " << mip << " is not in range [0, " << NUM_BLOOM_MIPS << ")");
        const bool isDownsamplePass = pass < NUM_BLOOM_LAYERS;
        const std::array<vk::ImageView, 1> attachments = {colorViews[!isDownsamplePass][mip]};

        const uint32_t inverseSize = 1u << mip;

        const vk::RenderPass renderPass =
            pass == NUM_BLOOM_PASSES - 1 ? createInfo.bloomGraphicsObjects.renderPasses.combine
            : pass < NUM_BLOOM_LAYERS	 ? createInfo.bloomGraphicsObjects.renderPasses.downsample
                                            : createInfo.bloomGraphicsObjects.renderPasses.upsample;

        const vk::FramebufferCreateInfo framebufferCreateInfo(
            {},
            renderPass,
            attachments.size(),
            attachments.data(),
            createInfo.swapchainExtent.width / inverseSize,
            createInfo.swapchainExtent.height / inverseSize,
            1
        );

        const vk::ResultValue<vk::Framebuffer> framebufferCreation =
            createInfo.device.createFramebuffer(framebufferCreateInfo);
        VULKAN_ENSURE_SUCCESS(framebufferCreation.result, "Can't create bloom framebuffer:");
        framebuffers[pass] = framebufferCreation.value;
    }

    const vk::DescriptorSet combineDescriptor =
        createInfo.bloomGraphicsObjects.pools.layer_combine
            .allocate(createInfo.device, createInfo.bloomGraphicsObjects.uniformLayouts.combine, 1)
            .front();
    const std::vector<vk::DescriptorSet> downsampleDescriptors =
        createInfo.bloomGraphicsObjects.pools.layer_downsample.allocate(
            createInfo.device, createInfo.bloomGraphicsObjects.uniformLayouts.downsample, NUM_BLOOM_LAYERS
        );
    const std::vector<vk::DescriptorSet> upsampleDescriptors =
        createInfo.bloomGraphicsObjects.pools.layer_upsample.allocate(
            createInfo.device, createInfo.bloomGraphicsObjects.uniformLayouts.upsample, NUM_BLOOM_LAYERS - 1
        );
    BloomGraphicsObjects::SwapchainObject::Descriptors descriptors = {.combine = combineDescriptor};
    std::copy(downsampleDescriptors.begin(), downsampleDescriptors.end(), descriptors.downsample.begin());
    std::copy(upsampleDescriptors.begin(), upsampleDescriptors.end(), descriptors.upsample.begin());

    writeBuffer.writeImage(
        descriptors.downsample[0],
        0,
        createInfo.colorBuffer.imageView,
        vk::DescriptorType::eCombinedImageSampler,
        createInfo.linearSampler,
        vk::ImageLayout::eShaderReadOnlyOptimal
    );
    for (size_t pass = 1; pass < NUM_BLOOM_LAYERS; pass++) {
        writeBuffer.writeImage(
            descriptors.downsample[pass],
            0,
            colorViews[0][getBloomSampleMip(pass)],
            vk::DescriptorType::eCombinedImageSampler,
            createInfo.linearSampler,
            vk::ImageLayout::eShaderReadOnlyOptimal
        );
    }
    for (size_t pass = NUM_BLOOM_LAYERS; pass < NUM_BLOOM_PASSES - 1; pass++) {
        const bool shouldSampleFirstImage = pass == NUM_BLOOM_LAYERS;
        writeBuffer.writeImage(
            descriptors.upsample[pass - NUM_BLOOM_LAYERS],
            0,
            colorViews[!shouldSampleFirstImage][getBloomSampleMip(pass)],
            vk::DescriptorType::eCombinedImageSampler,
            createInfo.linearSampler,
            vk::ImageLayout::eShaderReadOnlyOptimal
        );
        writeBuffer.writeImage(
            descriptors.upsample[pass - NUM_BLOOM_LAYERS],
            1,
            colorViews[0][getBloomRenderMip(pass)],
            vk::DescriptorType::eCombinedImageSampler,
            createInfo.linearSampler,
            vk::ImageLayout::eShaderReadOnlyOptimal
        );
        createInfo.bloomGraphicsObjects.buffers.upsample.bind(
            writeBuffer, descriptors.upsample[pass - NUM_BLOOM_LAYERS], 2
        );
    }
    writeBuffer.writeImage(
        descriptors.combine,
        0,	// previous mip
        colorViews[1][1],
        vk::DescriptorType::eCombinedImageSampler,
        createInfo.linearSampler,
        vk::ImageLayout::eShaderReadOnlyOptimal
    );
    writeBuffer.writeImage(
        descriptors.combine,
        1,	// current mip
        createInfo.colorBuffer.imageView,
        vk::DescriptorType::eCombinedImageSampler,
        createInfo.linearSampler,
        vk::ImageLayout::eShaderReadOnlyOptimal
    );

	createInfo.bloomGraphicsObjects.buffers.shared.update(
		BloomSharedBuffer{.baseMipSize = glm::vec2(createInfo.swapchainExtent.width, createInfo.swapchainExtent.height)}
	);
    createInfo.bloomGraphicsObjects.buffers.combine.bind(writeBuffer, descriptors.combine, 2);
	writeBuffer.flush(createInfo.device);

    return BloomGraphicsObjects::SwapchainObject{
        .colorBuffer = images,
        .colorMemory = memory,
        .colorViews = colorViews,
        .descriptors = descriptors,
        .framebuffers = framebuffers
    };
}

void destroy(BloomGraphicsObjects& objects, vk::Device device) {
	if (objects.swapchainObject.has_value()) destroyBloomSwapchainObject(objects, device);

	for (const vk::Pipeline& pipeline : objects.pipelines) device.destroyPipeline(pipeline);
	objects.buffers.combine.destroyBy(device);
	objects.buffers.shared.destroyBy(device);
	objects.buffers.upsample.destroyBy(device);
	device.destroyDescriptorPool(objects.pools.shared);
	objects.pools.layer_upsample.destroyBy(device);
	objects.pools.layer_downsample.destroyBy(device);
	objects.pools.layer_combine.destroyBy(device);

	device.destroyPipelineLayout(objects.pipelineLayouts.combine);
	device.destroyPipelineLayout(objects.pipelineLayouts.upsample);
	device.destroyPipelineLayout(objects.pipelineLayouts.downsample);

	device.destroyDescriptorSetLayout(objects.uniformLayouts.upsample);
	device.destroyDescriptorSetLayout(objects.uniformLayouts.downsample);
	device.destroyDescriptorSetLayout(objects.uniformLayouts.combine);
	device.destroyDescriptorSetLayout(objects.uniformLayouts.shared);
	device.destroyRenderPass(objects.renderPasses.combine);
	device.destroyRenderPass(objects.renderPasses.upsample);
	device.destroyRenderPass(objects.renderPasses.downsample);
}

void destroyBloomSwapchainObject(BloomGraphicsObjects& graphicsObjects, vk::Device device) {
	if (!graphicsObjects.swapchainObject.has_value()) return;

	graphicsObjects.pools.layer_upsample.clearPools(device);
	graphicsObjects.pools.layer_downsample.clearPools(device);
	graphicsObjects.pools.layer_combine.clearPools(device);

	const BloomGraphicsObjects::SwapchainObject& swapchainObject = graphicsObjects.swapchainObject.value();
    for (const vk::Framebuffer& framebuffer : swapchainObject.framebuffers) device.destroyFramebuffer(framebuffer);
    for (int image_index = 0; image_index < BloomGraphicsObjects::SwapchainObject::NUM_BUFFERS; image_index++) {
        for (const vk::ImageView& imageView : swapchainObject.colorViews[image_index])
            device.destroyImageView(imageView);
        device.destroyImage(swapchainObject.colorBuffer[image_index]);
        device.freeMemory(swapchainObject.colorMemory[image_index]);
    }

	graphicsObjects.swapchainObject = {};
}

}  // namespace graphics

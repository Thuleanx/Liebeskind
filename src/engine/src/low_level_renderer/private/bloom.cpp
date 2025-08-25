#include "bloom.h"

#include "core/logger/assert.h"
#include "core/logger/vulkan_ensures.h"
#include "descriptor.h"
#include "image.h"
#include "low_level_renderer/descriptor_write_buffer.h"
#include "low_level_renderer/shaders.h"
#include "pipeline.h"

namespace graphics {
void recordBloomRenderpass(
	Module& module,
	RenderSubmission& renderSubmission,
	vk::CommandBuffer buffer,
	uint32_t imageIndex
) {
	ASSERT(
		module.device.swapchain.has_value(),
		"Cannot record if the swapchain is empty"
	);
	ASSERT(
		module.device.bloom.swapchainObjects.has_value(),
		"Cannot record if the swapchain is empty"
	);

	ASSERT(
		module.device.bloom.swapchainObjects->size() > imageIndex,
		"Image index " << imageIndex << " exceeds bloom swapchain capacity of "
					   << module.device.bloom.swapchainObjects->size()
	);
	const BloomGraphicsObjects::SwapchainObject& bloomSwapchainObject =
		module.device.bloom.swapchainObjects.value()[imageIndex];

	for (size_t pass = 0; pass < NUM_BLOOM_PASSES; pass++) {
		const bool isCombinePass = pass == NUM_BLOOM_PASSES - 1;
		const size_t mip = getBloomLayerMip(pass);

		const vk::Viewport viewport(
			module.mainWindowExtent.offset.x,
			module.mainWindowExtent.offset.y,
			module.mainWindowExtent.extent.width / (1u << mip),
			module.mainWindowExtent.extent.height / (1u << mip),
			0.0f,
			1.0f
		);
		buffer.setViewport(0, 1, &viewport);

		const vk::Rect2D renderRect(
			module.mainWindowExtent.offset,
			{module.mainWindowExtent.extent.width / (1u << mip),
			 module.mainWindowExtent.extent.height / (1u << mip)}
		);
		buffer.setScissor(0, 1, &renderRect);
		const vk::RenderPass renderPass =
			isCombinePass ? module.device.bloom.renderPasses.combine
			: pass < NUM_BLOOM_LAYERS
				? module.device.bloom.renderPasses.downsample
				: module.device.bloom.renderPasses.upsample;
		const vk::PipelineLayout pipelineLayout =
			isCombinePass ? module.device.bloom.pipelineLayouts.combine
			: pass < NUM_BLOOM_LAYERS
				? module.device.bloom.pipelineLayouts.downsample
				: module.device.bloom.pipelineLayouts.upsample;

		constexpr std::array<vk::ClearValue, 3> clearColors{
			vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f)
		};
		const vk::RenderPassBeginInfo renderPassInfo(
			renderPass,
			bloomSwapchainObject.framebuffers[pass],
			renderRect,
			clearColors.size(),
			clearColors.data()
		);
		buffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
		buffer.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics,
			pipelineLayout,
			static_cast<int>(BloomDescriptorSetBindingPoint::eSwapchain),
			1,
			&module.device.bloom.descriptors.swapchain,
			0,
			nullptr
		);
		if (isCombinePass) {
			buffer.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				pipelineLayout,
				static_cast<int>(BloomDescriptorSetBindingPoint::eCombine),
				1,
				&bloomSwapchainObject.descriptors.combine,
				0,
				nullptr
			);
		} else {
			buffer.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				pipelineLayout,
				static_cast<int>(BloomDescriptorSetBindingPoint::eTexture),
				1,
				&bloomSwapchainObject.descriptors.texture[pass],
				0,
				nullptr
			);
		}

		buffer.bindPipeline(
			vk::PipelineBindPoint::eGraphics,
			module.device.bloom.pipelines[pass]
		);
		buffer.draw(6, 1, 0, 0);
		buffer.endRenderPass();
	}
}

BloomGraphicsObjects createBloomObjects(
	vk::Device device,
	vk::PhysicalDevice physicalDevice,
	ShaderStorage& shaders,
	vk::Format colorFormat
) {
	ASSERT(
		NUM_BLOOM_PASSES > 0,
		"Number of bloom passes cannot be 0. "
			<< "If not using bloom, then don't call this function."
	);

	const auto createRenderPass = [&](vk::ImageLayout attachmentInitialLayout) {
		const std::array<vk::AttachmentDescription, 1> attachments = {
			vk::AttachmentDescription(
				{},
				colorFormat,
				vk::SampleCountFlagBits::e1,
				vk::AttachmentLoadOp::eClear,
				vk::AttachmentStoreOp::eStore,
				vk::AttachmentLoadOp::eClear,
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
		const vk::RenderPassCreateInfo renderPassInfo(
			{},
			attachments.size(),
			attachments.data(),
			1,
			&mainSubpassDescriptions,
			0,
			nullptr
		);
		const vk::ResultValue<vk::RenderPass> renderPassCreation =
			device.createRenderPass(renderPassInfo);
		VULKAN_ENSURE_SUCCESS(
			renderPassCreation.result, "Can't create bloom renderpass:"
		);
		return renderPassCreation.value;
	};
	const BloomGraphicsObjects::RenderPasses renderPasses = {
		.downsample = createRenderPass(vk::ImageLayout::eUndefined),
		.upsample = createRenderPass(vk::ImageLayout::eShaderReadOnlyOptimal),
		.combine = createRenderPass(vk::ImageLayout::eUndefined)
	};

	constexpr std::array<vk::DescriptorSetLayoutBinding, 2> COMBINE_LAYOUT = {
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
	};
	constexpr std::array<vk::DescriptorSetLayoutBinding, 1> TEXTURE_LAYOUT = {
		vk::DescriptorSetLayoutBinding{
			0,	// binding
			vk::DescriptorType::eCombinedImageSampler,
			1,	// descriptor count
			vk::ShaderStageFlagBits::eFragment
		}
	};
	constexpr std::array<vk::DescriptorSetLayoutBinding, 1> SWAPCHAIN_LAYOUT = {
		vk::DescriptorSetLayoutBinding{
			0,	// binding
			vk::DescriptorType::eUniformBuffer,
			1,	// descriptor count
			vk::ShaderStageFlagBits::eFragment
		}
	};
	const BloomGraphicsObjects::SetLayouts setLayouts{
		.combine = createDescriptorLayout(device, COMBINE_LAYOUT),
		.texture = createDescriptorLayout(device, TEXTURE_LAYOUT),
		.swapchain = createDescriptorLayout(device, SWAPCHAIN_LAYOUT),
	};

	const std::array<vk::DescriptorSetLayout, 2> nonfinalLayouts = {
		setLayouts.texture, setLayouts.swapchain
	};
	const std::array<vk::DescriptorSetLayout, 2> finalLayouts = {
		setLayouts.combine, setLayouts.swapchain
	};
	const BloomGraphicsObjects::PipelineLayouts pipelineLayouts = {
		.downsample = createPipelineLayout(device, nonfinalLayouts, {}),
		.upsample = createPipelineLayout(device, nonfinalLayouts, {}),
		.combine = createPipelineLayout(device, finalLayouts, {}),
	};

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
	const std::array<float, 4> colorBlendingConstants = {
		0.0f, 0.0f, 0.0f, 0.0f
	};
	const std::array<vk::PipelineColorBlendAttachmentState, 1>
		colorBlendStates = {vk::PipelineColorBlendAttachmentState(
			vk::False,	// disable blend
			vk::BlendFactor::eSrcAlpha,
			vk::BlendFactor::eOneMinusSrcAlpha,
			vk::BlendOp::eAdd,
			vk::BlendFactor::eOne,
			vk::BlendFactor::eZero,
			vk::BlendOp::eAdd,
			vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
				vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
		)};
	const vk::PipelineColorBlendStateCreateInfo colorBlendingInfo(
		{},
		vk::False,
		vk::LogicOp::eCopy,
		colorBlendStates.size(),
		colorBlendStates.data(),
		colorBlendingConstants
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

	const ShaderID vertexShaderID = loadShaderFromFile(
		shaders, device, "shaders/entire_screen.vert.glsl.spv"
	);
	const ShaderID fragmentShaderID =
		loadShaderFromFile(shaders, device, "shaders/bloom.frag.glsl.spv");
	const ShaderID combineShaderID = loadShaderFromFile(
		shaders, device, "shaders/bloom_combine.frag.glsl.spv"
	);

	const vk::PipelineShaderStageCreateInfo vertexShaderInfo(
		{},
		vk::ShaderStageFlagBits::eVertex,
		getModule(shaders, vertexShaderID),
		"main",
		nullptr
	);

	std::array<vk::Pipeline, NUM_BLOOM_PASSES> pipelines;
	for (uint32_t subpass = 0; subpass < NUM_BLOOM_PASSES; subpass++) {
		const bool isUpsamplePass = subpass >= NUM_BLOOM_LAYERS;
		const bool isLastSubpass = subpass == NUM_BLOOM_PASSES - 1;

		const uint32_t divisions =
			subpass > 0 ? getBloomLayerMip(subpass - 1) : 0;
		const std::string kernelName = "main";
		const BloomSpecializationConstants constants = {
			.texelScale = static_cast<float>(1u << divisions),
			.sampleDistance = 0.5f
		};

		constexpr std::array<vk::SpecializationMapEntry, 2>
			SPECIALIZATION_INFO_ENTRY = {
				vk::SpecializationMapEntry{
					0,
					offsetof(BloomSpecializationConstants, sampleDistance),
					sizeof(float)
				},
				vk::SpecializationMapEntry{
					1,
					offsetof(BloomSpecializationConstants, texelScale),
					sizeof(float)
				}
			};

		const vk::SpecializationInfo specializationInfo{
			SPECIALIZATION_INFO_ENTRY.size(),
			SPECIALIZATION_INFO_ENTRY.data(),
			sizeof(constants),
			&constants
		};

		const vk::PipelineShaderStageCreateInfo fragmentShaderInfo(
			{},
			vk::ShaderStageFlagBits::eFragment,
			getModule(
				shaders, isLastSubpass ? combineShaderID : fragmentShaderID
			),
			kernelName.c_str(),
			&specializationInfo
		);

		const std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {
			vertexShaderInfo, fragmentShaderInfo
		};

		vk::PipelineLayout pipelineLayout;
		vk::RenderPass renderPass;
		if (isLastSubpass) {
			pipelineLayout = pipelineLayouts.combine;
			renderPass = renderPasses.combine;
		} else if (isUpsamplePass) {
			pipelineLayout = pipelineLayouts.upsample;
			renderPass = renderPasses.upsample;
		} else {
			pipelineLayout = pipelineLayouts.downsample;
			renderPass = renderPasses.downsample;
		}

		const vk::GraphicsPipelineCreateInfo pipelineCreateInfo(
			{},
			shaderStages.size(),
			shaderStages.data(),
			&vertexInputStateInfo,
			&inputAssemblyStateInfo,
			nullptr,  // no tesselation viewport
			&viewportStateInfo,
			&rasterizerCreateInfo,
			&multisamplingInfo,
			&depthStencilState,
			&colorBlendingInfo,
			&dynamicStateInfo,
			pipelineLayout,
			renderPass,
			0  // subpass = 0 since we use one subpass per renderpass
		);

		const vk::ResultValue<vk::Pipeline> pipelineCreation =
			device.createGraphicsPipeline(nullptr, pipelineCreateInfo);
		VULKAN_ENSURE_SUCCESS(
			pipelineCreation.result, "Can't create bloom pipeline:"
		);

		pipelines[subpass] = pipelineCreation.value;
	}

	constexpr std::array<vk::DescriptorPoolSize, 2> COMBINE_POOL_SIZES = {
		vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 2),
		vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1),
	};
	constexpr std::array<vk::DescriptorPoolSize, 1> TEXTURE_POOL_SIZES = {
		vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1),
	};
	constexpr std::array<vk::DescriptorPoolSize, 1> SWAPCHAIN_POOL_SIZES = {
		vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1),
	};

	const BloomGraphicsObjects::DescriptorPools pools = {
		.combine = DescriptorAllocator::create(
			device, COMBINE_POOL_SIZES, MAX_FRAMES_IN_FLIGHT
		),
		.texture = DescriptorAllocator::create(
			device, TEXTURE_POOL_SIZES, NUM_BLOOM_PASSES - 1
		),
		.swapchain = createDescriptorPool(device, 1, SWAPCHAIN_POOL_SIZES),
	};

	const std::vector<vk::DescriptorSet> swapchainDescriptors =
		createDescriptorSets(device, pools.swapchain, setLayouts.swapchain, 1);

	const BloomGraphicsObjects::Descriptors descriptors = {
		.swapchain = swapchainDescriptors.front()
	};

	// TODO: optimize this write buffer. It's using way too much memory.
	// Write buffers in general is good and perhaps we should have a global one
	DescriptorWriteBuffer writeBuffer;

	const BloomGraphicsObjects::DataBuffers buffers{
		.swapchain =
			UniformBuffer<BloomUniformBuffer>::create(device, physicalDevice, 1)
	};

	buffers.swapchain.bind(
		writeBuffer,
		descriptors.swapchain,
        0
	);
	writeBuffer.batchWrite(device);

	return BloomGraphicsObjects{
		.renderPasses = renderPasses,
		.setLayouts = setLayouts,
		.pipelineLayouts = pipelineLayouts,
		.pools = pools,
		.descriptors = descriptors,
		.buffers = buffers,
		.pipelines = pipelines,
		.swapchainObjects = {}
	};
}

std::vector<BloomGraphicsObjects::SwapchainObject> createBloomSwapchainObjects(
	BloomSwapchainObjectsCreateInfo createInfo
) {
	const size_t numObjects = createInfo.colorBuffers.size();
	std::vector<BloomGraphicsObjects::SwapchainObject> swapchainObjects;
	swapchainObjects.reserve(numObjects);

	DescriptorWriteBuffer writeBuffer;
	for (size_t objectIndex = 0; objectIndex < numObjects; objectIndex++) {
		const vk::Format imageFormat =
			createInfo.colorBuffers[objectIndex].format;
		const auto [image, memory] = Image::createImage(
			createInfo.device,
			createInfo.physicalDevice,
			createInfo.swapchainExtent.width,
			createInfo.swapchainExtent.height,
			imageFormat,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eColorAttachment |
				vk::ImageUsageFlagBits::eSampled,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			vk::SampleCountFlagBits::e1,
			NUM_BLOOM_MIPS
		);

		std::array<vk::ImageView, NUM_BLOOM_MIPS> colorViews;
		for (size_t i = 0; i < NUM_BLOOM_MIPS; i++) {
			colorViews[i] = Image::createImageView(
				createInfo.device,
				image,
				imageFormat,
				vk::ImageAspectFlagBits::eColor,
				i,
				1
			);
		}
		std::array<vk::Framebuffer, NUM_BLOOM_PASSES> framebuffers;
		for (size_t pass = 0; pass < NUM_BLOOM_PASSES; pass++) {
			const size_t mip = getBloomLayerMip(pass);
			ASSERT(
				mip >= 0 && mip < NUM_BLOOM_MIPS,
				"Mip " << mip << " is not in range [0, " << NUM_BLOOM_MIPS
					   << ")"
			);
			const std::array<vk::ImageView, 1> attachments = {colorViews[mip]};

			const uint32_t inverseSize = 1u << mip;

			const vk::RenderPass renderPass =
				pass == NUM_BLOOM_PASSES - 1
					? createInfo.bloomGraphicsObjects.renderPasses.combine
				: pass < NUM_BLOOM_LAYERS
					? createInfo.bloomGraphicsObjects.renderPasses.downsample
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
			VULKAN_ENSURE_SUCCESS(
				framebufferCreation.result, "Can't create bloom framebuffer:"
			);
			framebuffers[pass] = framebufferCreation.value;
		}

		const vk::DescriptorSet combineDescriptor =
			createInfo.bloomGraphicsObjects.pools.combine
				.allocate(
					createInfo.device,
					createInfo.bloomGraphicsObjects.setLayouts.combine,
					1
				)
				.front();
		BloomGraphicsObjects::SwapchainObject::Descriptors descriptors = {
			.combine = combineDescriptor
		};
		const std::vector<vk::DescriptorSet> textureSets =
			createInfo.bloomGraphicsObjects.pools.texture.allocate(
				createInfo.device,
				createInfo.bloomGraphicsObjects.setLayouts.texture,
				NUM_BLOOM_PASSES - 1
			);
		std::copy(
			textureSets.begin(), textureSets.end(), descriptors.texture.begin()
		);

		constexpr size_t textureBinding = 0;
		static_assert(NUM_BLOOM_LAYERS >= 1);
		writeBuffer.writeImage(
			descriptors.texture[0],
			textureBinding,
			createInfo.colorBuffers[objectIndex].imageView,
			vk::DescriptorType::eCombinedImageSampler,
			createInfo.linearSampler,
			vk::ImageLayout::eShaderReadOnlyOptimal
		);
		for (size_t pass = 1; pass < NUM_BLOOM_PASSES - 1; pass++)
			writeBuffer.writeImage(
				descriptors.texture[pass],
				textureBinding,
				colorViews[getBloomLayerMip(pass - 1)],
				vk::DescriptorType::eCombinedImageSampler,
				createInfo.linearSampler,
				vk::ImageLayout::eShaderReadOnlyOptimal
			);
		static_assert(NUM_BLOOM_MIPS > 1);
		writeBuffer.writeImage(
			descriptors.combine,
			textureBinding,
			colorViews[1],
			vk::DescriptorType::eCombinedImageSampler,
			createInfo.linearSampler,
			vk::ImageLayout::eShaderReadOnlyOptimal
		);
		constexpr size_t combineBinding = 1;
		writeBuffer.writeImage(
			descriptors.combine,
			combineBinding,
			createInfo.colorBuffers[objectIndex].imageView,
			vk::DescriptorType::eCombinedImageSampler,
			createInfo.linearSampler,
			vk::ImageLayout::eShaderReadOnlyOptimal
		);

		swapchainObjects.push_back(BloomGraphicsObjects::SwapchainObject{
			.colorBuffer = image,
			.colorMemory = memory,
			.colorViews = colorViews,
			.descriptors = descriptors,
			.framebuffers = framebuffers
		});
	}
	writeBuffer.batchWrite(createInfo.device);

	createInfo.bloomGraphicsObjects.buffers.swapchain.update(BloomUniformBuffer{
		.swapchainExtent = glm::vec2(
			createInfo.swapchainExtent.height, createInfo.swapchainExtent.width
		)
	});

	return swapchainObjects;
}

void destroy(BloomGraphicsObjects& objects, vk::Device device) {
	if (objects.swapchainObjects.has_value())
		destroyBloomSwapchainObjects(objects, device);

	objects.buffers.swapchain.destroyBy(device);
	for (const vk::Pipeline& pipeline : objects.pipelines)
		device.destroyPipeline(pipeline);
	device.destroyDescriptorPool(objects.pools.swapchain);
	objects.pools.texture.destroyBy(device);
	objects.pools.combine.destroyBy(device);
	device.destroyPipelineLayout(objects.pipelineLayouts.combine);
	device.destroyPipelineLayout(objects.pipelineLayouts.upsample);
	device.destroyPipelineLayout(objects.pipelineLayouts.downsample);
	device.destroyDescriptorSetLayout(objects.setLayouts.combine);
	device.destroyDescriptorSetLayout(objects.setLayouts.texture);
	device.destroyDescriptorSetLayout(objects.setLayouts.swapchain);
	device.destroyRenderPass(objects.renderPasses.combine);
	device.destroyRenderPass(objects.renderPasses.upsample);
	device.destroyRenderPass(objects.renderPasses.downsample);
}

void destroyBloomSwapchainObjects(
	BloomGraphicsObjects& graphicsObjects, vk::Device device
) {
	if (!graphicsObjects.swapchainObjects.has_value()) return;

	graphicsObjects.pools.texture.clearPools(device);
	graphicsObjects.pools.combine.clearPools(device);

	for (const BloomGraphicsObjects::SwapchainObject& swapchainObject :
		 graphicsObjects.swapchainObjects.value()) {
		for (const vk::Framebuffer& framebuffer : swapchainObject.framebuffers)
			device.destroyFramebuffer(framebuffer);
		for (const vk::ImageView& imageView : swapchainObject.colorViews)
			device.destroyImageView(imageView);
		device.destroyImage(swapchainObject.colorBuffer);
		device.freeMemory(swapchainObject.colorMemory);
	}

    graphicsObjects.swapchainObjects = {};
}

}  // namespace graphics

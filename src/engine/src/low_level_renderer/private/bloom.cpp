#include "bloom.h"

#include "core/logger/assert.h"
#include "core/logger/vulkan_ensures.h"
#include "descriptor.h"
#include "image.h"
#include "low_level_renderer/descriptor_write_buffer.h"
#include "low_level_renderer/shaders.h"

namespace graphics {
BloomSwapchainData createBloomData(
	vk::Device device,
	vk::PhysicalDevice physicalDevice,
	const BloomSettings& settings,
	BloomPipeline& pipeline,
	vk::Format imageFormat,
	vk::Extent2D swapchainExtent,
	size_t swapchainSize
) {
	std::vector<BloomSwapchainData::Attachment> attachments;
	attachments.reserve(swapchainSize);

	for (size_t i = 0; i < swapchainSize; i++) {
		const auto [image, memory] = Image::createImage(
			device,
			physicalDevice,
			swapchainExtent.width,
			swapchainExtent.height,
			imageFormat,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eColorAttachment |
				vk::ImageUsageFlagBits::eSampled,
			vk::MemoryPropertyFlagBits::eDeviceLocal,
			vk::SampleCountFlagBits::e1,
			NUM_BLOOM_LAYERS
		);

		std::array<vk::ImageView, NUM_BLOOM_LAYERS> mipViews;
		for (int i = 0; i < NUM_BLOOM_LAYERS; i++) {
			mipViews[i] = Image::createImageView(
				device,
				image,
				imageFormat,
				vk::ImageAspectFlagBits::eColor,
				i,
				1
			);
		}

		std::array<vk::DescriptorSet, NUM_BLOOM_PASSES> textureDescriptors;
        {
            const std::vector<vk::DescriptorSet> textureDescriptorsVec =
                pipeline.textureDescriptorAllocator.allocate(
                    device, pipeline.textureSetLayout, NUM_BLOOM_PASSES
                );

            for (int i = 0; i < NUM_BLOOM_PASSES; i++) {
                ASSERT(textureDescriptorsVec[i] != 0, "Allocated texture descriptors are invalid");
                textureDescriptors[i] = textureDescriptorsVec[i];
            }
        }

		attachments.push_back(BloomSwapchainData::Attachment{
			.image = image,
			.memory = memory,
			.mipViews = mipViews,
			.textureDescriptors = textureDescriptors
		});
	}

	return BloomSwapchainData{.attachments = attachments};
}

void destroy(const BloomSwapchainData& bloomData, vk::Device device) {
	for (const BloomSwapchainData::Attachment& attachment :
		 bloomData.attachments) {
		for (const vk::ImageView& view : attachment.mipViews)
			device.destroyImageView(view);
		device.destroyImage(attachment.image);
		device.freeMemory(attachment.memory);
	}
}

vk::RenderPass createBloomRenderpass(
	vk::Device device, vk::Format colorAttachmentFormat
) {
	// +1 includes the input color attachment
	const uint32_t NUM_ATTACHMENTS = NUM_BLOOM_LAYERS + 1;
	// NUM_BLOOM_LAYERS downsample passes and then the same number of upsample
	// passes
	const uint32_t NUM_SUBPASSES = 2 * NUM_BLOOM_LAYERS;
	std::array<vk::AttachmentDescription, NUM_ATTACHMENTS> attachments;
	// input color attachment
	attachments[0] = vk::AttachmentDescription{
		{},
		colorAttachmentFormat,
		vk::SampleCountFlagBits::e1,
		vk::AttachmentLoadOp::eLoad,
		vk::AttachmentStoreOp::eStore,
		vk::AttachmentLoadOp::eClear,
		vk::AttachmentStoreOp::eDontCare,
		// we expect this to be in eColorAttachmentOptimal since the previous
		// renderpass just finished writing to it
		vk::ImageLayout::eColorAttachmentOptimal,
		vk::ImageLayout::eColorAttachmentOptimal
	};
	for (uint32_t i = 1; i < NUM_BLOOM_LAYERS + 1; i++) {
		attachments[i] = vk::AttachmentDescription{
			{},
			colorAttachmentFormat,
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eColorAttachmentOptimal
		};
	}

	// we declare these here because SubpassDescription has pointers to these
	// references so the attachment reference must live until the renderpass is
	// created completely
	std::array<std::vector<vk::AttachmentReference>, NUM_SUBPASSES>
		subpassInputRefs;
	std::array<std::vector<vk::AttachmentReference>, NUM_SUBPASSES>
		subpassColorRefs;
	std::array<vk::SubpassDescription, NUM_SUBPASSES> subpassDescriptions;
	std::array<vk::SubpassDependency, NUM_SUBPASSES> subpassDependencies;

	// downsample subpasses
	for (uint32_t i = 0; i < NUM_BLOOM_LAYERS; i++) {
		subpassInputRefs[i] = {
			vk::AttachmentReference{i, vk::ImageLayout::eShaderReadOnlyOptimal}
		};
		subpassColorRefs[i] = {vk::AttachmentReference{
			i + 1, vk::ImageLayout::eColorAttachmentOptimal
		}};
	}

	// upsample subpasses
	for (uint32_t i = NUM_BLOOM_LAYERS; i < NUM_SUBPASSES; i++) {
		subpassInputRefs[i] = {vk::AttachmentReference{
			NUM_SUBPASSES - i, vk::ImageLayout::eShaderReadOnlyOptimal
		}};
		subpassColorRefs[i] = {vk::AttachmentReference{
			NUM_SUBPASSES - i - 1, vk::ImageLayout::eColorAttachmentOptimal
		}};
	}

	for (uint32_t i = 0; i < NUM_SUBPASSES; i++) {
		subpassDescriptions[i] = vk::SubpassDescription(
			{},
			vk::PipelineBindPoint::eGraphics,
			subpassInputRefs[i].size(),
			subpassInputRefs[i].data(),
			subpassColorRefs[i].size(),
			subpassColorRefs[i].data(),
			nullptr,
			nullptr
		);

		if (i == 0) {
			subpassDependencies[i] = vk::SubpassDependency(
				vk::SubpassExternal,
				0,
				vk::PipelineStageFlagBits::eColorAttachmentOutput |
					vk::PipelineStageFlagBits::eEarlyFragmentTests,
				vk::PipelineStageFlagBits::eColorAttachmentOutput |
					vk::PipelineStageFlagBits::eEarlyFragmentTests,
				{},
				vk::AccessFlagBits::eColorAttachmentWrite |
					vk::AccessFlagBits::eDepthStencilAttachmentWrite
			);
		} else {
			subpassDependencies[i] = vk::SubpassDependency(
				i - 1,
				i,
				vk::PipelineStageFlagBits::eColorAttachmentOutput,
				vk::PipelineStageFlagBits::eFragmentShader,
				vk::AccessFlagBits::eColorAttachmentWrite,
				vk::AccessFlagBits::eShaderRead
			);
		}
	}

	const vk::RenderPassCreateInfo renderPassInfo(
		{},
		attachments.size(),
		attachments.data(),
		subpassDescriptions.size(),
		subpassDescriptions.data(),
		subpassDependencies.size(),
		subpassDependencies.data()
	);
	const vk::ResultValue<vk::RenderPass> renderPassCreation =
		device.createRenderPass(renderPassInfo);
	VULKAN_ENSURE_SUCCESS(
		renderPassCreation.result, "Can't create bloom renderpass:"
	);
	return renderPassCreation.value;
}

BloomPipeline createBloomPipeline(
	ShaderStorage& shaders,
	vk::Device device,
	vk::PhysicalDevice physicalDevice,
	const RenderPassData& renderPasses
) {
	const vk::DescriptorSetLayout textureSetLayout =
		createDescriptorLayout(device, BLOOM_TEXTURE_BINDINGS);
	const vk::DescriptorSetLayout swapchainSetLayout =
		createDescriptorLayout(device, BLOOM_SWAPCHAIN_BINDINGS);

	const vk::DescriptorPool swapchainDescriptorPool =
		createDescriptorPool(device, 1, BLOOM_SWAPCHAIN_DESCRIPTOR_POOL_SIZES);

	const DescriptorAllocator textureDescriptorAllocator =
		DescriptorAllocator::create(
			device, BLOOM_TEXTURE_DESCRIPTOR_POOL_SIZES, NUM_BLOOM_PASSES
		);

	const ShaderID vertexShaderID =
		loadShaderFromFile(shaders, device, "shaders/entire_screen.vert.glsl");
	const ShaderID fragmentShaderID =
		loadShaderFromFile(shaders, device, "shaders/bloom.frag.glsl");

	const vk::PipelineShaderStageCreateInfo vertexShaderInfo(
		{},
		vk::ShaderStageFlagBits::eVertex,
		getModule(shaders, vertexShaderID),
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

	const std::array<vk::DescriptorSetLayout, 2> setLayouts = {
		textureSetLayout, swapchainSetLayout
	};
	const vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
		{}, setLayouts.size(), setLayouts.data(), 0, nullptr
	);
	const vk::ResultValue<vk::PipelineLayout> pipelineLayoutCreation =
		device.createPipelineLayout(pipelineLayoutInfo);
	VULKAN_ENSURE_SUCCESS(
		pipelineLayoutCreation.result, "Can't create bloom pipeline layout:"
	);
	const vk::PipelineLayout pipelineLayout = pipelineLayoutCreation.value;

	const UniformBuffer<BloomUniformBuffer> ubo =
		UniformBuffer<BloomUniformBuffer>::create(device, physicalDevice, 1);
	const vk::DescriptorSet swapchainDescriptor = [&]() {
		const vk::DescriptorSetAllocateInfo allocateInfo(
			swapchainDescriptorPool, 1, &swapchainSetLayout
		);
		const vk::ResultValue<std::vector<vk::DescriptorSet>>
			descriptorSetCreation = device.allocateDescriptorSets(allocateInfo);
		VULKAN_ENSURE_SUCCESS(
			descriptorSetCreation.result,
			"Unexpected error when creating descriptor sets"
		);
		ASSERT(
			descriptorSetCreation.value.size() == 1,
			"Requested " << 1 << " bloom swapchain descriptor sets, got "
						 << descriptorSetCreation.value.size()
		);
		return descriptorSetCreation.value[0];
	}();

	{  // Bind ubo to swapchain descriptor
		DescriptorWriteBuffer tempWriteBuffer;
		ubo.bind(tempWriteBuffer, swapchainDescriptor, 0);
		tempWriteBuffer.batchWrite(device);
	}

	std::array<vk::Pipeline, NUM_BLOOM_PASSES> pipelines;
	for (uint32_t subpass = 0; subpass < NUM_BLOOM_PASSES; subpass++) {
		const bool isUpsamplePass = subpass >= NUM_BLOOM_LAYERS;

		const uint32_t divisions =
			isUpsamplePass ? NUM_BLOOM_PASSES - subpass : subpass;
		const std::string kernelName =
			isUpsamplePass ? "upsample" : "downsample";

		const BloomSpecializationConstants constants = {
			.texelScale = static_cast<float>(1u << divisions),
			.sampleDistance = 0.5f
		};

		const vk::SpecializationInfo specializationInfo{
			BLOOM_SPECIALIZATION_INFO.size(),
			BLOOM_SPECIALIZATION_INFO.data(),
			sizeof(constants),
			&constants
		};

		const vk::PipelineShaderStageCreateInfo fragmentShaderInfo(
			{},
			vk::ShaderStageFlagBits::eFragment,
			getModule(shaders, fragmentShaderID),
			kernelName.c_str(),
			&specializationInfo
		);

		const std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {
			vertexShaderInfo, fragmentShaderInfo
		};

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
			pipelineLayoutCreation.value,
			renderPasses.bloomPass,
			subpass
		);

		const vk::ResultValue<vk::Pipeline> pipelineCreation =
			device.createGraphicsPipeline(nullptr, pipelineCreateInfo);
		VULKAN_ENSURE_SUCCESS(
			pipelineCreation.result, "Can't create bloom pipeline:"
		);

		pipelines[subpass] = pipelineCreation.value;
	}

	return BloomPipeline{
		.pipelines = pipelines,
		.layout = pipelineLayout,
		.textureSetLayout = textureSetLayout,
		.swapchainSetLayout = swapchainSetLayout,
		.swapchainDescriptorPool = swapchainDescriptorPool,
		.textureDescriptorAllocator = textureDescriptorAllocator,
		.ubo = ubo
	};
}

void updateBloomUniform(
	BloomPipeline& bloomPipeline, vk::Extent2D swapchainExtent
) {
	bloomPipeline.ubo.update(BloomUniformBuffer{
		.swapchainExtent =
			glm::vec2(swapchainExtent.height, swapchainExtent.width)
	});
}

void destroy(const BloomPipeline& bloomPipeline, vk::Device device) {
	for (const vk::Pipeline& pipeline : bloomPipeline.pipelines)
		device.destroyPipeline(pipeline);
	bloomPipeline.textureDescriptorAllocator.destroyBy(device);
	bloomPipeline.ubo.destroyBy(device);
	device.destroyPipelineLayout(bloomPipeline.layout);
	device.destroyDescriptorPool(bloomPipeline.swapchainDescriptorPool);
	device.destroy(bloomPipeline.textureSetLayout);
	device.destroy(bloomPipeline.swapchainSetLayout);
}

}  // namespace graphics

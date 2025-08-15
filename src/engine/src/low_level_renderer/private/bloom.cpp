#include "bloom.h"

#include "core/logger/vulkan_ensures.h"
#include "image.h"
#include "low_level_renderer/shaders.h"

namespace graphics {
BloomData createBloomData(
	const GraphicsDeviceInterface& graphics,
	const BloomSettings& settings,
	vk::Extent2D swapchainExtent,
	uint32_t swapchainSize
) {
	std::vector<BloomData::Attachment> colorAttachments;
	colorAttachments.reserve(swapchainSize);

	const vk::Format imageFormat = graphics.renderPasses.colorAttachmentFormat;

	for (size_t i = 0; i < swapchainSize; i++) {
		const auto [image, memory] = Image::createImage(
			graphics.device,
			graphics.physicalDevice,
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

		std::array<vk::ImageView, 3> mipViews;
		for (int i = 0; i < NUM_BLOOM_LAYERS; i++) {
			mipViews[i] = Image::createImageView(
				graphics.device,
				image,
				imageFormat,
				vk::ImageAspectFlagBits::eColor,
				i,
				1
			);
		}
	}

	return BloomData{.colorAttachments = colorAttachments};
}

void destroy(const BloomData& bloomData, vk::Device device) {
	for (const BloomData::Attachment& attachment : bloomData.colorAttachments) {
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
	const RenderPassData& renderPasses
) {
	const vk::ResultValue<vk::DescriptorSetLayout> setLayoutCreation =
		device.createDescriptorSetLayout(
			{{},
			 static_cast<uint32_t>(BLOOM_BINDINGS.size()),
			 BLOOM_BINDINGS.data()}
		);
	VULKAN_ENSURE_SUCCESS(
		setLayoutCreation.result,
		"Can't create postProcessing descriptor set layout"
	);
	const vk::DescriptorSetLayout setLayout = setLayoutCreation.value;
	const DescriptorAllocator setAllocator = DescriptorAllocator::create(
		device, BLOOM_DESCRIPTOR_POOL_SIZES, MAX_FRAMES_IN_FLIGHT
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

	const vk::PipelineShaderStageCreateInfo downscaleShaderInfo(
		{},
		vk::ShaderStageFlagBits::eFragment,
		getModule(shaders, fragmentShaderID),
		"downsample"
	);

	const vk::PipelineShaderStageCreateInfo upscaleShaderInfo(
		{},
		vk::ShaderStageFlagBits::eFragment,
		getModule(shaders, fragmentShaderID),
		"upsample"
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

	const std::array<vk::DescriptorSetLayout, 1> setLayouts = {setLayout};
	const vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
		{}, setLayouts.size(), setLayouts.data(), 0, nullptr
	);
	const vk::ResultValue<vk::PipelineLayout> pipelineLayoutCreation =
		device.createPipelineLayout(pipelineLayoutInfo);
	VULKAN_ENSURE_SUCCESS(
		pipelineLayoutCreation.result, "Can't create bloom pipeline layout:"
	);
	const vk::PipelineLayout pipelineLayout = pipelineLayoutCreation.value;
	const std::array<vk::PipelineShaderStageCreateInfo, 2> upsampleStages = {
		vertexShaderInfo, upscaleShaderInfo
	};
	const std::array<vk::PipelineShaderStageCreateInfo, 2> downsampleStages = {
		vertexShaderInfo, downscaleShaderInfo
	};

	const uint32_t NUM_SUBPASSES = 2 * NUM_BLOOM_LAYERS;
    std::array<vk::Pipeline, NUM_SUBPASSES> pipelines;
    for (uint32_t subpass = 0; subpass < NUM_SUBPASSES; subpass++) {
        const bool isUpsamplePass = subpass >= NUM_BLOOM_LAYERS;

        const vk::GraphicsPipelineCreateInfo pipelineCreateInfo(
            {},
            isUpsamplePass ? upsampleStages.size() : downsampleStages.size(),
            isUpsamplePass ? upsampleStages.data() : downsampleStages.data(),
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

    return BloomPipeline {
        .pipelines = pipelines,
        .layout = pipelineLayout,
        .descriptorLayout = setLayout,
        .allocator = setAllocator
    };
}

void destroy(const BloomPipeline& bloomPipeline, vk::Device device) {
    for (const vk::Pipeline& pipeline : bloomPipeline.pipelines)
        device.destroyPipeline(pipeline);
    device.destroyPipelineLayout(bloomPipeline.layout);
    bloomPipeline.allocator.destroyBy(device);
	device.destroy(bloomPipeline.descriptorLayout);
}

}  // namespace graphics

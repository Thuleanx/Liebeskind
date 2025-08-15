#include "low_level_renderer/renderpass_data.h"

#include "core/logger/vulkan_ensures.h"
#include "private/bloom.h"

namespace graphics {
RenderPassData RenderPassData::create(
	vk::Device device,
	vk::Format colorAttachmentFormat,
	vk::SurfaceFormatKHR swapchainImageFormat,
	vk::Format swapchainDepthFormat,
	vk::SampleCountFlagBits msaaSampleCount
) {
	const vk::RenderPass mainPass = createMainRenderPass(
		device, colorAttachmentFormat, swapchainDepthFormat, msaaSampleCount
	);
	const vk::RenderPass postProcessingPass =
		createPostProcessingRenderPass(device, swapchainImageFormat.format);
    const vk::RenderPass bloomPass =
        createBloomRenderpass(device, swapchainImageFormat.format);

	return {
		.colorAttachmentFormat = colorAttachmentFormat,
		.swapchainColorFormat = swapchainImageFormat,
		.depthAttachmentFormat = swapchainDepthFormat,
		.multisampleAntialiasingSampleCount = msaaSampleCount,
		.mainPass = mainPass,
        .bloomPass = bloomPass,
		.postProcessingPass = postProcessingPass,
	};
}

vk::RenderPass createMainRenderPass(
	vk::Device device,
	vk::Format colorAttachmentImageFormat,
	vk::Format depthFormat,
	vk::SampleCountFlagBits msaaSampleCount
) {
	const std::array<vk::AttachmentDescription, 3> attachments = {
		vk::AttachmentDescription{// multisample color
								  {},
								  colorAttachmentImageFormat,
								  msaaSampleCount,
								  vk::AttachmentLoadOp::eClear,
								  vk::AttachmentStoreOp::eStore,
								  vk::AttachmentLoadOp::eClear,
								  vk::AttachmentStoreOp::eDontCare,
								  vk::ImageLayout::eUndefined,
								  vk::ImageLayout::eColorAttachmentOptimal
		},
		vk::AttachmentDescription{
			// depth
			{},
			depthFormat,
			msaaSampleCount,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eDontCare,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eShaderReadOnlyOptimal
		},
		vk::AttachmentDescription{// intermediate color attachment
								  {},
								  colorAttachmentImageFormat,
								  vk::SampleCountFlagBits::e1,
								  vk::AttachmentLoadOp::eClear,
								  vk::AttachmentStoreOp::eStore,
								  vk::AttachmentLoadOp::eClear,
								  vk::AttachmentStoreOp::eDontCare,
								  vk::ImageLayout::eUndefined,
								  vk::ImageLayout::eShaderReadOnlyOptimal
		}
	};
	const std::array<vk::AttachmentReference, 1> subpassColorRef = {
		vk::AttachmentReference{0, vk::ImageLayout::eColorAttachmentOptimal}
	};
	const std::array<vk::AttachmentReference, 1> subpassResolveRef = {
		vk::AttachmentReference{2, vk::ImageLayout::eColorAttachmentOptimal}
	};
	const std::array<vk::AttachmentReference, 1> subpassDepthRef = {
		vk::AttachmentReference{
			1, vk::ImageLayout::eDepthStencilAttachmentOptimal
		}
	};
	const vk::SubpassDescription subpass(
		{},
		vk::PipelineBindPoint::eGraphics,
		0,
		nullptr,
		subpassColorRef.size(),
		subpassColorRef.data(),
		subpassResolveRef.data(),
		subpassDepthRef.data()
	);

	const vk::SubpassDependency subpassDependency(
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
	const vk::RenderPassCreateInfo renderPassInfo(
		{},
		attachments.size(),
		attachments.data(),
		1,
		&subpass,
		1,
		&subpassDependency
	);
	const vk::ResultValue<vk::RenderPass> renderPassCreation =
		device.createRenderPass(renderPassInfo);
	VULKAN_ENSURE_SUCCESS(
		renderPassCreation.result, "Can't create renderpass:"
	);
	return renderPassCreation.value;
}

vk::RenderPass createPostProcessingRenderPass(
	vk::Device device, vk::Format swapchainImageFormat
) {
	const std::array<vk::AttachmentDescription, 1> attachments = {
		vk::AttachmentDescription{// swapchain color attachment
								  {},
								  swapchainImageFormat,
								  vk::SampleCountFlagBits::e1,
								  vk::AttachmentLoadOp::eClear,
								  vk::AttachmentStoreOp::eStore,
								  vk::AttachmentLoadOp::eClear,
								  vk::AttachmentStoreOp::eDontCare,
								  vk::ImageLayout::eUndefined,
								  vk::ImageLayout::eColorAttachmentOptimal
		}
	};
	const std::array<vk::AttachmentReference, 1> subpassColor = {
		vk::AttachmentReference{0, vk::ImageLayout::eColorAttachmentOptimal}
	};
	const vk::SubpassDescription subpass(
		{},
		vk::PipelineBindPoint::eGraphics,
		0,
		nullptr,
		subpassColor.size(),
		subpassColor.data(),
		nullptr,
		nullptr
	);
	const vk::SubpassDependency subpassDependency(
		vk::SubpassExternal,
		0,
		vk::PipelineStageFlagBits::eColorAttachmentOutput |
			vk::PipelineStageFlagBits::eEarlyFragmentTests,
		vk::PipelineStageFlagBits::eColorAttachmentOutput |
			vk::PipelineStageFlagBits::eEarlyFragmentTests,
		{},
		vk::AccessFlagBits::eColorAttachmentWrite
	);
	const vk::RenderPassCreateInfo renderPassInfo(
		{},
		attachments.size(),
		attachments.data(),
		1,
		&subpass,
		1,
		&subpassDependency
	);
	const vk::ResultValue<vk::RenderPass> renderPassCreation =
		device.createRenderPass(renderPassInfo);
	VULKAN_ENSURE_SUCCESS(
		renderPassCreation.result, "Can't create renderpass:"
	);
	return renderPassCreation.value;
}

void destroy(const RenderPassData& renderPassData, vk::Device device) {
    device.destroyRenderPass(renderPassData.mainPass);
    device.destroyRenderPass(renderPassData.postProcessingPass);
}

};	// namespace graphics

#pragma once

#include <vulkan/vulkan.hpp>

namespace graphics {

struct RenderPassData {
	vk::Format colorAttachmentFormat;
	vk::SurfaceFormatKHR swapchainColorFormat;
	vk::Format depthAttachmentFormat;
	vk::SampleCountFlagBits multisampleAntialiasingSampleCount;

	vk::RenderPass mainPass;
	vk::RenderPass postProcessingPass;

   public:
	static RenderPassData create(
		vk::Device device,
		vk::Format colorAttachmentFormat,
		vk::SurfaceFormatKHR swapchainImageFormat,
		vk::Format swapchainDepthFormat,
		vk::SampleCountFlagBits msaaSampleCount
	);
};

vk::RenderPass createMainRenderPass(
	vk::Device device,
	vk::Format colorAttachmentImageFormat,
	vk::Format depthFormat,
	vk::SampleCountFlagBits msaaSampleCount
);

vk::RenderPass createPostProcessingRenderPass(
	vk::Device device, vk::Format swapchainImageFormat
);

void destroy(const RenderPassData& renderPassData, vk::Device device);

}  // namespace graphics

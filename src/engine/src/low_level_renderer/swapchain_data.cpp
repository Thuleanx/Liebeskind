#include "low_level_renderer/swapchain_data.h"

#include "core/logger/logger.h"
#include "core/logger/vulkan_ensures.h"
#include "low_level_renderer/graphics_device_interface.h"
#include "private/graphics_device_helper.h"
#include "private/image.h"
#include "private/swapchain.h"

namespace graphics {
float SwapchainData::getAspectRatio() const {
	return extent.width / (float)extent.height;
}

SwapchainData GraphicsDeviceInterface::createSwapchain() const {
	QueueFamilyIndices queueFamily =
		QueueFamilyIndices::findQueueFamilies(physicalDevice, surface);

	const vk::PresentModeKHR presentMode = Swapchain::chooseSwapPresentMode(
		Swapchain::getSupportedPresentModes(physicalDevice, surface)
	);
	const vk::SurfaceCapabilitiesKHR surfaceCapability =
		Swapchain::getSurfaceCapability(physicalDevice, surface);
	const vk::Extent2D extent =
		Swapchain::chooseSwapExtent(surfaceCapability, window);
	const vk::SurfaceFormatKHR colorAttachmentFormat =
		Swapchain::getSuitableColorAttachmentFormat(physicalDevice, surface);
	uint32_t imageCount = surfaceCapability.minImageCount + 1;

	if (surfaceCapability.maxImageCount > 0)
		imageCount = std::min(imageCount, surfaceCapability.maxImageCount);

	const bool shouldUseExclusiveSharingMode =
		queueFamily.presentFamily.value() == queueFamily.graphicsFamily.value();
	const uint32_t queueFamilyIndices[] = {
		queueFamily.presentFamily.value(), queueFamily.graphicsFamily.value()
	};
	const vk::SharingMode sharingMode = shouldUseExclusiveSharingMode
											? vk::SharingMode::eExclusive
											: vk::SharingMode::eConcurrent;
	const vk::SwapchainCreateInfoKHR swapchainCreateInfo(
		{},
		surface,
		imageCount,
		colorAttachmentFormat.format,
		colorAttachmentFormat.colorSpace,
		extent,
		1,
		vk::ImageUsageFlagBits::eColorAttachment,
		sharingMode,
		shouldUseExclusiveSharingMode ? 0 : 2,
		shouldUseExclusiveSharingMode ? nullptr : queueFamilyIndices,
		surfaceCapability.currentTransform,
		vk::CompositeAlphaFlagBitsKHR::eOpaque,
		presentMode,
		true,  // clipped
		nullptr
	);

	const vk::ResultValue<vk::SwapchainKHR> swapchainCreateResult =
		device.createSwapchainKHR(swapchainCreateInfo, nullptr);
	VULKAN_ENSURE_SUCCESS(
		swapchainCreateResult.result, "Can't create swapchain:"
	);
	const vk::SwapchainKHR swapchain = swapchainCreateResult.value;

	LLOG_INFO << "Swapchain created with format "
			  << to_string(colorAttachmentFormat.format) << " and extent "
			  << extent.width << " x " << extent.height;
	const auto colorAttachmentsGet = device.getSwapchainImagesKHR(swapchain);
	VULKAN_ENSURE_SUCCESS(
		colorAttachmentsGet.result, "Can't get swapchain images:"
	);
	const std::vector<vk::Image> colorAttachments = colorAttachmentsGet.value;
	std::vector<vk::ImageView> colorAttachmentViews(colorAttachments.size());
	for (size_t i = 0; i < colorAttachments.size(); i++) {
		colorAttachmentViews[i] = Image::createImageView(
			device,
			colorAttachments[i],
			colorAttachmentFormat.format,
			vk::ImageAspectFlagBits::eColor
		);
	}

	LLOG_INFO << "Created swapchain images and image views";

	const vk::Format depthAttachmentFormat =
		Swapchain::getSuitableDepthAttachmentFormat(physicalDevice);

	const size_t swapchainSize = colorAttachments.size();

	std::vector<Texture> depthAttachments;
	depthAttachments.reserve(swapchainSize);
	for (size_t i = 0; i < swapchainSize; i++) {
		depthAttachments.push_back(createTexture(
			device,
			physicalDevice,
			depthAttachmentFormat,
			extent.width,
			extent.height,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eDepthStencilAttachment,
			vk::ImageAspectFlagBits::eDepth,
            msaaSampleCount
		));
		transitionLayout(
			depthAttachments.back(),
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eDepthStencilAttachmentOptimal,
			device,
			commandPool,
			graphicsQueue
		);
	}

	std::vector<Texture> multisampleColorAttachments;
	multisampleColorAttachments.reserve(swapchainSize);
	for (size_t i = 0; i < swapchainSize; i++) {
		multisampleColorAttachments.push_back(createTexture(
			device,
			physicalDevice,
			colorAttachmentFormat.format,
			extent.width,
			extent.height,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eColorAttachment,
			vk::ImageAspectFlagBits::eColor,
            msaaSampleCount
		));
	}

	std::vector<vk::Framebuffer> swapchainFramebuffers(swapchainSize);

	for (size_t i = 0; i < swapchainSize; i++) {
		ASSERT(
			colorAttachmentViews[i],
			"Swapchain image at location " << i << " is null"
		);
		const std::array<vk::ImageView, 3> attachments = {
			multisampleColorAttachments[i].imageView,
			depthAttachments[i].imageView,
			colorAttachmentViews[i],
		};
		const vk::FramebufferCreateInfo framebufferCreateInfo(
			{},
			renderPass,
			static_cast<uint32_t>(attachments.size()),
			attachments.data(),
			extent.width,
			extent.height,
			1
		);
		const vk::ResultValue<vk::Framebuffer> framebufferCreation =
			device.createFramebuffer(framebufferCreateInfo);
		VULKAN_ENSURE_SUCCESS(
			framebufferCreation.result, "Can't create framebuffer:"
		);
		swapchainFramebuffers[i] = framebufferCreation.value;
	}

	return SwapchainData{
		.swapchain = swapchain,
		.extent = extent,
		.colorAttachments = colorAttachments,
		.colorAttachmentViews = colorAttachmentViews,
        .multisampleColorAttachments = multisampleColorAttachments,
		.depthAttachments = depthAttachments,
		.framebuffers = swapchainFramebuffers,
		.colorAttachmentFormat = colorAttachmentFormat.format,
		.depthAttachmentFormat = depthAttachmentFormat,
		.imageCount = imageCount,
	};
}

void GraphicsDeviceInterface::destroy(SwapchainData& swapchainData) const {
	for (const vk::Framebuffer& framebuffer : swapchainData.framebuffers)
		device.destroyFramebuffer(framebuffer);

	for (const vk::ImageView& imageView : swapchainData.colorAttachmentViews)
		device.destroyImageView(imageView);

	graphics::destroy(swapchainData.depthAttachments, device);
    graphics::destroy(swapchainData.multisampleColorAttachments, device);

	device.destroySwapchainKHR(swapchainData.swapchain);
}
}  // namespace graphics

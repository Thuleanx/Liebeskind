#include "low_level_renderer/swapchain_data.h"

#include "core/logger/logger.h"
#include "core/logger/vulkan_ensures.h"
#include "low_level_renderer/graphics_device_interface.h"
#include "private/bloom.h"
#include "private/image.h"
#include "private/swapchain.h"

namespace graphics {
float SwapchainData::getAspectRatio() const {
	return extent.width / (float)extent.height;
}

SwapchainData GraphicsDeviceInterface::createSwapchain() {
	QueueFamilyIndices queueFamily =
		QueueFamilyIndices::findQueueFamilies(physicalDevice, surface);

	const vk::PresentModeKHR presentMode = Swapchain::chooseSwapPresentMode(
		Swapchain::getSupportedPresentModes(physicalDevice, surface)
	);
	const vk::SurfaceCapabilitiesKHR surfaceCapability =
		Swapchain::getSurfaceCapability(physicalDevice, surface);
	const vk::Extent2D extent =
		Swapchain::chooseSwapExtent(surfaceCapability, window);

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
		MAX_FRAMES_IN_FLIGHT,
		renderPasses.swapchainColorFormat.format,
		renderPasses.swapchainColorFormat.colorSpace,
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
			  << to_string(renderPasses.swapchainColorFormat.format)
			  << " and extent " << extent.width << " x " << extent.height;
	const auto colorAttachmentsGet = device.getSwapchainImagesKHR(swapchain);
	VULKAN_ENSURE_SUCCESS(
		colorAttachmentsGet.result, "Can't get swapchain images:"
	);
	const std::vector<vk::Image> colorAttachments = colorAttachmentsGet.value;
	std::vector<vk::ImageView> colorAttachmentViews(colorAttachments.size());

	const vk::Format colorFormat = renderPasses.swapchainColorFormat.format;
	for (size_t i = 0; i < colorAttachments.size(); i++) {
		constexpr uint32_t mipLevelBase = 0;
		constexpr uint32_t colorAttachmentMipLevels = 1;
		colorAttachmentViews[i] = Image::createImageView(
			device,
			colorAttachments[i],
			colorFormat,
			vk::ImageAspectFlagBits::eColor,
			mipLevelBase,
			colorAttachmentMipLevels
		);
	}

	LLOG_INFO << "Created swapchain images and image views";

	const size_t swapchainSize = colorAttachments.size();

	std::vector<Texture> depthAttachments;
	depthAttachments.reserve(swapchainSize);
	std::vector<Texture> multisampleColorAttachments;
	multisampleColorAttachments.reserve(swapchainSize);
	std::vector<Texture> intermediateColorAttachments;
	intermediateColorAttachments.reserve(swapchainSize);

	DescriptorWriteBuffer writeBuffer;
	for (size_t i = 0; i < swapchainSize; i++) {
		constexpr uint32_t depthMipLevels = 1;
		depthAttachments.push_back(createTexture(
			device,
			physicalDevice,
			renderPasses.depthAttachmentFormat,
			extent.width,
			extent.height,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eDepthStencilAttachment |
				vk::ImageUsageFlagBits::eSampled,
			vk::ImageAspectFlagBits::eDepth,
			renderPasses.multisampleAntialiasingSampleCount,
			depthMipLevels
		));
		transitionLayout(
			depthAttachments.back(),
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eDepthStencilAttachmentOptimal,
			device,
			commandPool,
			graphicsQueue
		);
		constexpr uint32_t multisampleMipLevels = 1;
		multisampleColorAttachments.push_back(createTexture(
			device,
			physicalDevice,
			renderPasses.colorAttachmentFormat,
			extent.width,
			extent.height,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eColorAttachment,
			vk::ImageAspectFlagBits::eColor,
			renderPasses.multisampleAntialiasingSampleCount,
			multisampleMipLevels
		));
		constexpr uint32_t intermediateMipLevels = 1;
		intermediateColorAttachments.push_back(createTexture(
			device,
			physicalDevice,
			renderPasses.colorAttachmentFormat,
			extent.width,
			extent.height,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eColorAttachment |
				vk::ImageUsageFlagBits::eSampled,
			vk::ImageAspectFlagBits::eColor,
			vk::SampleCountFlagBits::e1,
			intermediateMipLevels
		));
		bindTextureToDescriptor(
			depthAttachments[i].imageView,
			frameDatas[i].postProcessingDescriptor,
			1,
			samplers.point,
			writeBuffer,
			vk::ImageLayout::eShaderReadOnlyOptimal
		);
	}

	const BloomSwapchainObjectsCreateInfo bloomSwapchainObjectsCreateInfo = {
		.device = device,
		.physicalDevice = physicalDevice,
		.colorBuffers = intermediateColorAttachments,
		.swapchainExtent = extent,
		.bloomGraphicsObjects = bloom,
		.linearSampler = samplers.linearClearBorder
	};

	bloom.swapchainObjects =
		createBloomSwapchainObjects(bloomSwapchainObjectsCreateInfo);
    for (size_t i = 0; i < swapchainSize; i++) {
		bindTextureToDescriptor(
            // intermediateColorAttachments[i].imageView,
			bloom.swapchainObjects.value()[i].colorViews[1].front(),
			frameDatas[i].postProcessingDescriptor,
			0,
			samplers.point,
			writeBuffer,
			vk::ImageLayout::eShaderReadOnlyOptimal
		);
    }

	std::vector<vk::Framebuffer> mainFramebuffers(swapchainSize);
	std::vector<vk::Framebuffer> postProcessingFramebuffers(swapchainSize);
	for (size_t i = 0; i < swapchainSize; i++) {
		ASSERT(
			colorAttachmentViews[i],
			"Swapchain image at location " << i << " is null"
		);
		ASSERT(
			depthAttachments[i].imageView,
			"Swapchain depth image at location " << i << " is null"
		);
		const std::array<vk::ImageView, 3> mainAttachments = {
			multisampleColorAttachments[i].imageView,
			depthAttachments[i].imageView,
			intermediateColorAttachments[i].imageView,
		};
		const std::array<vk::ImageView, 1> postProcessingAttachments = {
			colorAttachmentViews[i]
		};

		const vk::FramebufferCreateInfo mainFramebufferCreateInfo(
			{},
			renderPasses.mainPass,
			mainAttachments.size(),
			mainAttachments.data(),
			extent.width,
			extent.height,
			1
		);

		const vk::FramebufferCreateInfo postProcessingFramebufferCreateInfo(
			{},
			renderPasses.postProcessingPass,
			postProcessingAttachments.size(),
			postProcessingAttachments.data(),
			extent.width,
			extent.height,
			1
		);

		{  // Create main framebuffer
			const vk::ResultValue<vk::Framebuffer> mainFramebufferCreation =
				device.createFramebuffer(mainFramebufferCreateInfo);
			VULKAN_ENSURE_SUCCESS(
				mainFramebufferCreation.result, "Can't create main framebuffer:"
			);
			mainFramebuffers[i] = mainFramebufferCreation.value;
		}

		{  // Create post processing framebuffers
			const vk::ResultValue<vk::Framebuffer>
				postProcessingFramebufferCreation =
					device.createFramebuffer(postProcessingFramebufferCreateInfo
					);
			VULKAN_ENSURE_SUCCESS(
				postProcessingFramebufferCreation.result,
				"Can't create post processing framebuffer:"
			);
			postProcessingFramebuffers[i] =
				postProcessingFramebufferCreation.value;
		}
	}

	writeBuffer.batchWrite(device);

	return SwapchainData{
		.swapchain = swapchain,
		.extent = extent,
		.colorAttachments = colorAttachments,
		.colorAttachmentViews = colorAttachmentViews,
		.intermediateColorAttachments = intermediateColorAttachments,
		.multisampleColorAttachments = multisampleColorAttachments,
		.depthAttachments = depthAttachments,
		.mainFramebuffers = mainFramebuffers,
		.postProcessingFramebuffers = postProcessingFramebuffers
	};
}

void GraphicsDeviceInterface::destroy(SwapchainData& swapchainData) {
	for (const vk::Framebuffer& framebuffer : swapchainData.mainFramebuffers)
		device.destroyFramebuffer(framebuffer);

	for (const vk::Framebuffer& framebuffer :
		 swapchainData.postProcessingFramebuffers)
		device.destroyFramebuffer(framebuffer);

	for (const vk::ImageView& imageView : swapchainData.colorAttachmentViews)
		device.destroyImageView(imageView);

	graphics::destroy(swapchainData.intermediateColorAttachments, device);
	graphics::destroy(swapchainData.depthAttachments, device);
	graphics::destroy(swapchainData.multisampleColorAttachments, device);

	device.destroySwapchainKHR(swapchainData.swapchain);
}
}  // namespace graphics

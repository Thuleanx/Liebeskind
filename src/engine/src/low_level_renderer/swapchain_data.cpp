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
		queueFamily.presentFamily.value() == queueFamily.graphicsAndComputeFamily.value();
	const uint32_t queueFamilyIndices[] = {
		queueFamily.presentFamily.value(), queueFamily.graphicsAndComputeFamily.value()
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
			  << " and extent " << extent.width << " x " << extent.height << " with " << swapchain;
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
            vk::ImageViewType::e2D,
			colorFormat,
			vk::ImageAspectFlagBits::eColor,
			mipLevelBase,
			colorAttachmentMipLevels
		);
	}

	LLOG_INFO << "Created swapchain images and image views";

	const size_t swapchainSize = colorAttachments.size();

	const Texture depthAttachment = [&]() {
        constexpr uint32_t depthMipLevels = 1;
        Texture depthAttachment = createTexture(
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
        );
        transitionLayout(
            depthAttachment,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthStencilAttachmentOptimal,
            device,
            commandPool,
            graphicsAndComputeQueue
        );
        return depthAttachment;
    }();

	const Texture multisampleColorAttachment = [&]() {
        constexpr uint32_t multisampleMipLevels = 1;
        return createTexture(
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
        );
    }();

	const Texture intermediateColorAttachment = [&]() {
        constexpr uint32_t intermediateMipLevels = 1;
        return createTexture(
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
        );
    }();

    {
        LLOG_INFO << "Binding depth attachments to post processing samplers";
        DescriptorWriteBuffer writeBuffer;
        for (size_t i = 0; i < frameDatas.size(); i++) {

            bindTextureToDescriptor(
                depthAttachment.imageView,
                frameDatas[i].postProcessingDescriptor,
                1,
                samplers.point,
                writeBuffer,
                vk::ImageLayout::eShaderReadOnlyOptimal
            );
        }

        writeBuffer.flush(device);
    }

	const BloomSwapchainObjectCreateInfo bloomSwapchainObjectsCreateInfo = {
		.device = device,
		.physicalDevice = physicalDevice,
		.colorBuffer = intermediateColorAttachment,
		.swapchainExtent = extent,
		.bloomGraphicsObjects = bloom,
		.linearSampler = samplers.linearClearBorder
	};

    {
        DescriptorWriteBuffer writeBuffer;
        LLOG_INFO << "Creating bloom swapchain objects";
        bloom.swapchainObject =
            createBloomSwapchainObject(bloomSwapchainObjectsCreateInfo);
        LLOG_INFO << "Created bloom swapchain objects, binding...";
        for (size_t i = 0; i < frameDatas.size(); i++) {
            bindTextureToDescriptor(
                // intermediateColorAttachments[i].imageView,
                bloom.swapchainObject.value().colorViews[1].front(),
                frameDatas[i].postProcessingDescriptor,
                0,
                samplers.point,
                writeBuffer,
                vk::ImageLayout::eShaderReadOnlyOptimal
            );
        }
	    writeBuffer.flush(device);
    }

	const vk::Framebuffer mainFramebuffer = [&]() {
        ASSERT(
            depthAttachment.imageView,
            "Swapchain depth image is null"
        );
        const std::array<vk::ImageView, 3> mainAttachments = {
            multisampleColorAttachment.imageView,
            depthAttachment.imageView,
            intermediateColorAttachment.imageView,
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

        const vk::ResultValue<vk::Framebuffer> mainFramebufferCreation =
            device.createFramebuffer(mainFramebufferCreateInfo);
        VULKAN_ENSURE_SUCCESS(
            mainFramebufferCreation.result, "Can't create main framebuffer:"
        );
        return mainFramebufferCreation.value;
    }();


	std::vector<vk::Framebuffer> postProcessingFramebuffers(swapchainSize);
	for (size_t i = 0; i < swapchainSize; i++) {
		ASSERT(
			colorAttachmentViews[i],
			"Swapchain image at location " << i << " is null"
		);
		const std::array<vk::ImageView, 1> postProcessingAttachments = {
			colorAttachmentViews[i]
		};


		const vk::FramebufferCreateInfo postProcessingFramebufferCreateInfo(
			{},
			renderPasses.postProcessingPass,
			postProcessingAttachments.size(),
			postProcessingAttachments.data(),
			extent.width,
			extent.height,
			1
		);


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

    std::vector<vk::Semaphore> submitSemaphores(swapchainSize);
    for (size_t i = 0; i < swapchainSize; i++) {
	    vk::SemaphoreCreateInfo semaphoreCreateInfo;
		const vk::ResultValue<vk::Semaphore> submitSemaphoreCreation =
			device.createSemaphore({});
		VULKAN_ENSURE_SUCCESS(
			submitSemaphoreCreation.result, "Can't create semaphore:"
		);
        submitSemaphores[i] = submitSemaphoreCreation.value;
    }

	return SwapchainData{
		.swapchain = swapchain,
		.extent = extent,
		.colorAttachments = colorAttachments,
		.colorAttachmentViews = colorAttachmentViews,
		.intermediateColor = intermediateColorAttachment,
		.multisampleColor = multisampleColorAttachment,
		.depth = depthAttachment,
		.mainFramebuffer = mainFramebuffer,
		.postProcessingFramebuffers = postProcessingFramebuffers,
        .submitSemaphores = submitSemaphores
	};
}

void GraphicsDeviceInterface::destroy(SwapchainData& swapchainData) {
    for (const vk::Semaphore submitSemaphore : swapchainData.submitSemaphores)
		device.destroySemaphore(submitSemaphore);

	device.destroyFramebuffer(swapchainData.mainFramebuffer);

	for (const vk::Framebuffer& framebuffer :
		 swapchainData.postProcessingFramebuffers)
		device.destroyFramebuffer(framebuffer);

	for (const vk::ImageView& imageView : swapchainData.colorAttachmentViews)
		device.destroyImageView(imageView);

	graphics::destroy(std::array{swapchainData.intermediateColor, swapchainData.depth, swapchainData.multisampleColor}, device);
	device.destroySwapchainKHR(swapchainData.swapchain);
}
}  // namespace graphics

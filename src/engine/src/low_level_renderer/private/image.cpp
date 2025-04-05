#include "image.h"

#include "buffer.h"
#include "command.h"
#include "core/logger/vulkan_ensures.h"

std::tuple<vk::Image, vk::DeviceMemory> Image::createImage(
    const vk::Device &device,
    const vk::PhysicalDevice &physicalDevice,
    uint32_t width,
    uint32_t height,
    vk::Format format,
    vk::ImageTiling tiling,
    vk::ImageUsageFlags usage,
    vk::MemoryPropertyFlags properties,
    vk::SampleCountFlagBits sampleCount,
    uint32_t mipLevels
) {
    // To generate the mip maps, we need to blit the image onto itself, hence
    // this usage flag
    if (mipLevels > 0)
        usage |= vk::ImageUsageFlagBits::eTransferDst |
                 vk::ImageUsageFlagBits::eTransferSrc;
    const vk::ImageCreateInfo imageCreateInfo(
        {},
        vk::ImageType::e2D,
        format,
        vk::Extent3D(
            static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1
        ),
        mipLevels,                    // mip levels
        1,                            // array layers
        sampleCount,
        tiling,
        usage,
        vk::SharingMode::eExclusive,
        0,
        nullptr,
        vk::ImageLayout::eUndefined
    );
    const vk::ResultValue<vk::Image> imageCreation =
        device.createImage(imageCreateInfo);
    VULKAN_ENSURE_SUCCESS(imageCreation.result, "Unable to create image");

    const vk::Image image = imageCreation.value;
    const vk::MemoryRequirements memoryRequirements =
        device.getImageMemoryRequirements(image);
    const std::optional<uint32_t> suitableMemoryType =
        Buffer::findSuitableMemoryType(
            physicalDevice.getMemoryProperties(),
            memoryRequirements.memoryTypeBits,
            properties
        );
    ASSERT(
        suitableMemoryType.has_value(),
        "Can't find suitable memory type for requirement "
            << memoryRequirements.memoryTypeBits << " with property: "
            << vk::to_string(vk::MemoryPropertyFlagBits::eDeviceLocal)
    );
    const vk::MemoryAllocateInfo memoryAllocateInfo(
        memoryRequirements.size, suitableMemoryType.value()
    );
    const vk::ResultValue<vk::DeviceMemory> memoryAllocation =
        device.allocateMemory(memoryAllocateInfo);
    VULKAN_ENSURE_SUCCESS(
        memoryAllocation.result, "Can't allocate memory for image"
    );
    const vk::DeviceMemory deviceMemory = memoryAllocation.value;
    VULKAN_ENSURE_SUCCESS_EXPR(
        device.bindImageMemory(image, deviceMemory, 0),
        "Can't bind image and image memory"
    );
    return std::make_tuple(image, deviceMemory);
}

void Image::transitionImageLayout(
    const vk::Device &device,
    const vk::CommandPool &commandPool,
    const vk::Queue &graphicsQueue,
    const vk::Image &image,
    vk::Format format,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    uint32_t mipLevels
) {
    const vk::CommandBuffer commandBuffer =
        Command::beginSingleCommand(device, commandPool);

    vk::AccessFlags sourceAccessMask;
    vk::AccessFlags destinationAccessMask;
    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    if (oldLayout == vk::ImageLayout::eUndefined &&
        newLayout == vk::ImageLayout::eTransferDstOptimal) {
        sourceAccessMask = {};
        destinationAccessMask = vk::AccessFlagBits::eTransferWrite;
        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
               newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        sourceAccessMask = vk::AccessFlagBits::eTransferWrite;
        destinationAccessMask = vk::AccessFlagBits::eShaderRead;
        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else if (oldLayout == vk::ImageLayout::eUndefined &&
               newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        sourceAccessMask = {};
        destinationAccessMask =
            vk::AccessFlagBits::eDepthStencilAttachmentRead |
            vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    } else {
        ASSERT(false, "Unsupported layout transition");
        return;
    }

    vk::ImageAspectFlags imageAspect;
    if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        imageAspect = vk::ImageAspectFlagBits::eDepth;
        if (Image::hasStencilComponent(format))
            imageAspect |= vk::ImageAspectFlagBits::eStencil;
    } else {
        imageAspect = vk::ImageAspectFlagBits::eColor;
    }

    const vk::ImageMemoryBarrier barrier(
        sourceAccessMask,
        destinationAccessMask,
        oldLayout,
        newLayout,
        vk::QueueFamilyIgnored,
        vk::QueueFamilyIgnored,
        image,
        vk::ImageSubresourceRange(imageAspect, 0, mipLevels, 0, 1)
    );

    commandBuffer.pipelineBarrier(
        sourceStage, destinationStage, {}, 0, nullptr, 0, nullptr, 1, &barrier
    );

    Command::submitSingleCommand(
        device, graphicsQueue, commandPool, commandBuffer
    );
}

void Image::copyBufferToImage(
    const vk::Device &device,
    const vk::CommandPool &commandPool,
    const vk::Queue &graphicsQueue,
    const vk::Buffer &sourceBuffer,
    const vk::Image &destinationImage,
    uint32_t width,
    uint32_t height
) {
    const vk::CommandBuffer commandBuffer =
        Command::beginSingleCommand(device, commandPool);

    const vk::BufferImageCopy copyInfo(
        0,
        // the next two values are buffer row & height sizes. 0 indicates that
        // data is tightly packed
        0,
        0,
        vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
        vk::Offset3D(0, 0, 0),
        vk::Extent3D(width, height, 1)
    );

    commandBuffer.copyBufferToImage(
        sourceBuffer,
        destinationImage,
        vk::ImageLayout::eTransferDstOptimal,
        1,
        &copyInfo
    );

    Command::submitSingleCommand(
        device, graphicsQueue, commandPool, commandBuffer
    );
}

vk::ImageView Image::createImageView(
    const vk::Device &device,
    const vk::Image &image,
    vk::Format imageFormat,
    vk::ImageAspectFlags imageAspect
) {
    const vk::ImageViewCreateInfo imageViewInfo(
        {},
        image,
        vk::ImageViewType::e2D,
        imageFormat,
        {},
        vk::ImageSubresourceRange(imageAspect, 0, 1, 0, 1)
    );
    const vk::ResultValue<vk::ImageView> imageViewCreation =
        device.createImageView(imageViewInfo);
    VULKAN_ENSURE_SUCCESS(imageViewCreation.result, "Can't create image view");
    return imageViewCreation.value;
}
std::optional<vk::Format> Image::findSupportedFormat(
    const vk::PhysicalDevice &physicalDevice,
    const std::vector<vk::Format> &candidates,
    vk::ImageTiling imageTiling,
    vk::FormatFeatureFlags features
) {
    for (vk::Format format : candidates) {
        vk::FormatProperties props = physicalDevice.getFormatProperties(format);
        if (imageTiling == vk::ImageTiling::eLinear &&
            (props.linearTilingFeatures & features) == features)
            return format;
        if (imageTiling == vk::ImageTiling::eOptimal &&
            (props.optimalTilingFeatures & features) == features)
            return format;
    }
    return std::nullopt;
}

void Image::generateMipMaps(
	vk::Device device,
	vk::CommandPool commandPool,
	vk::Queue graphicsQueue,
	vk::Image image,
	uint32_t width,
	uint32_t height,
	uint32_t mipLevels
) {
	vk::CommandBuffer commandBuffer =
		Command::beginSingleCommand(device, commandPool);

	vk::ImageMemoryBarrier mipBarrier(
		vk::AccessFlagBits::eTransferWrite,
		vk::AccessFlagBits::eTransferRead,
		vk::ImageLayout::eTransferDstOptimal,
		vk::ImageLayout::eTransferSrcOptimal,
		vk::QueueFamilyIgnored,
		vk::QueueFamilyIgnored,
		image,
		vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
	);

	vk::ImageMemoryBarrier toShaderBarrier(
		vk::AccessFlagBits::eTransferRead,
		vk::AccessFlagBits::eShaderRead,
		vk::ImageLayout::eTransferSrcOptimal,
		vk::ImageLayout::eShaderReadOnlyOptimal,
		vk::QueueFamilyIgnored,
		vk::QueueFamilyIgnored,
		image,
		vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
	);

	uint32_t mipWidth = width;
	uint32_t mipHeight = height;

	for (uint32_t i = 1; i < mipLevels; i++) {
		mipBarrier.subresourceRange.setBaseMipLevel(i - 1);
	    toShaderBarrier.subresourceRange.setBaseMipLevel(i - 1);

		commandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eTransfer,
			{},
			0,
			nullptr,
			0,
			nullptr,
			1,
			&mipBarrier
		);

		const int nextMipWidth = std::max(mipWidth / 2, {1});
		const int nextMipHeight = std::max(mipHeight / 2, {1});

		const vk::ImageBlit imageBlit(
			vk::ImageSubresourceLayers(
				vk::ImageAspectFlagBits::eColor, i - 1, 0, 1
			),
			std::array<vk::Offset3D, 2>{
				{{0, 0, 0},
				 {static_cast<int>(mipWidth), static_cast<int>(mipHeight), 1}}
			},
			vk::ImageSubresourceLayers(
				vk::ImageAspectFlagBits::eColor, i, 0, 1
			),
			std::array<vk::Offset3D, 2>{
				{{0, 0, 0}, {nextMipWidth, nextMipHeight, 1}}
			}
		);

		commandBuffer.blitImage(
			image,
			vk::ImageLayout::eTransferSrcOptimal,
			image,
			vk::ImageLayout::eTransferDstOptimal,
			1,
			&imageBlit,
			vk::Filter::eLinear
		);

		commandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eFragmentShader,
			{},
			0,
			nullptr,
			0,
			nullptr,
			1,
			&toShaderBarrier
		);

		mipWidth = nextMipWidth;
		mipHeight = nextMipHeight;
	}

	toShaderBarrier.subresourceRange.setBaseMipLevel(mipLevels - 1);
    toShaderBarrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);

	commandBuffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eTransfer,
		vk::PipelineStageFlagBits::eFragmentShader,
		{},
		0,
		nullptr,
		0,
		nullptr,
		1,
		&toShaderBarrier
	);

	Command::submitSingleCommand(
		device, graphicsQueue, commandPool, commandBuffer
	);
}

bool Image::hasStencilComponent(vk::Format format) {
    return format == vk::Format::eD32SfloatS8Uint ||
           format == vk::Format::eD24UnormS8Uint;
}

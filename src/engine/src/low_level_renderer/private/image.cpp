#include "image.h"

#include "buffer.h"
#include "command.h"
#include "helpful_defines.h"

std::tuple<vk::Image, vk::DeviceMemory> Image::createImage(
    const vk::Device &device,
    const vk::PhysicalDevice &physicalDevice,
    uint32_t width,
    uint32_t height,
    vk::Format format,
    vk::ImageTiling tiling,
    vk::ImageUsageFlags usage,
    vk::MemoryPropertyFlags properties
) {
    const vk::ImageCreateInfo imageCreateInfo(
        {},
        vk::ImageType::e2D,
        format,
        vk::Extent3D(
            static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1
        ),
        1,                            // mip levels
        1,                            // array layers
        vk::SampleCountFlagBits::e1,  // sample count
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
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout
) {
    const vk::CommandBuffer commandBuffer =
        Command::beginSingleCommand(device, commandPool);

    vk::AccessFlagBits sourceAccessMask;
    vk::AccessFlagBits destinationAccessMask;
    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
        sourceAccessMask = {};
        destinationAccessMask = vk::AccessFlagBits::eTransferWrite;
        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        sourceAccessMask = vk::AccessFlagBits::eTransferWrite;
        destinationAccessMask = vk::AccessFlagBits::eShaderRead;
        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else {
        ASSERT(false, "Unsupported layout transition");
        return;
    }

    const vk::ImageMemoryBarrier barrier(
        sourceAccessMask,
        destinationAccessMask,
        oldLayout,
        newLayout,
        vk::QueueFamilyIgnored,
        vk::QueueFamilyIgnored,
        image,
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
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

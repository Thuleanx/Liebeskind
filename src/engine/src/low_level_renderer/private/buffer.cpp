#include "buffer.h"

#include "command.h"
#include "helpful_defines.h"

namespace Buffer {

std::tuple<vk::Buffer, vk::DeviceMemory> create(
    const vk::Device& device,
    const vk::PhysicalDevice& physicalDevice,
    const vk::DeviceSize size,
    const vk::BufferUsageFlags usage,
    const vk::MemoryPropertyFlags properties
) {
    const vk::BufferCreateInfo bufferInfo(
        {}, size, usage, vk::SharingMode::eExclusive
    );
    const vk::ResultValue<vk::Buffer> bufferCreation =
        device.createBuffer(bufferInfo);
    VULKAN_ENSURE_SUCCESS(bufferCreation.result, "Can't create buffer:");
    const vk::Buffer buffer = bufferCreation.value;
    const vk::MemoryRequirements memoryRequirements =
        device.getBufferMemoryRequirements(buffer);
    const std::optional<uint32_t> memoryTypeIndex = findSuitableMemoryType(
        physicalDevice.getMemoryProperties(),
        memoryRequirements.memoryTypeBits,
        properties
    );
    const vk::MemoryAllocateInfo allocateInfo(
        memoryRequirements.size, memoryTypeIndex.value()
    );
    const vk::ResultValue<vk::DeviceMemory> deviceMemoryAllocation =
        device.allocateMemory(allocateInfo);
    VULKAN_ENSURE_SUCCESS(
        deviceMemoryAllocation.result, "Failed to allocate vertex buffer memory"
    );
    const vk::DeviceMemory deviceMemory = deviceMemoryAllocation.value;
    VULKAN_ENSURE_SUCCESS_EXPR(
        device.bindBufferMemory(buffer, deviceMemory, 0),
        "Failed to bind buffer memory"
    );
    return std::make_tuple(buffer, deviceMemory);
}

std::optional<uint32_t> findSuitableMemoryType(
    const vk::PhysicalDeviceMemoryProperties& memoryProperties,
    const uint32_t typeFilter,
    const vk::MemoryPropertyFlags properties
) {
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
        bool doesPropFitTypeFilter = typeFilter & (1 << i);
        bool doesPropFitPropertyFlags =
            (memoryProperties.memoryTypes[i].propertyFlags & properties) ==
            properties;

        if (doesPropFitTypeFilter && doesPropFitPropertyFlags) return i;
    }

    return {};
}

void copyBuffer(
    const vk::Device& device,
    const vk::CommandPool& commandPool,
    const vk::Queue& submitQueue,
    const vk::Buffer srcBuffer,
    const vk::Buffer dstBuffer,
    const vk::DeviceSize size
) {
    const vk::CommandBuffer commandBuffer =
        Command::beginSingleCommand(device, commandPool);

    vk::BufferCopy copyRegion(
        0,    // srcOffset
        0,    // dstOffset
        size  // size
    );

    commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);

    Command::submitSingleCommand(
        device, submitQueue, commandPool, commandBuffer
    );
}

}  // namespace Buffer

#pragma once

#include "buffer.h"

// not neccessary, only for LSP
#include <vector>
#include <vulkan/vulkan.hpp>
#include "helpful_defines.h"

namespace Buffer {
template <typename T>
std::tuple<vk::Buffer, vk::DeviceMemory> loadToBuffer(
    const vk::Device& device,
    const vk::PhysicalDevice& physicalDevice,
    const vk::CommandPool& commandPool,
    const vk::Queue& graphicsQueue,
    const std::vector<T>& data,
    vk::BufferUsageFlagBits usage
) {
    const uint32_t bufferSize = sizeof(T) * data.size();

    const auto [stagingBuffer, stagingBufferMemory] = Buffer::create(
        device,
        physicalDevice,
        bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent
    );

    {  // memcpy the data onto staging buffer
        const vk::ResultValue<void*> mappedMemory =
            device.mapMemory(stagingBufferMemory, 0, bufferSize, {});
        VULKAN_ENSURE_SUCCESS(
            mappedMemory.result, "Can't map staging buffer memory"
        );
        memcpy(
            mappedMemory.value, data.data(), static_cast<size_t>(bufferSize)
        );
        device.unmapMemory(stagingBufferMemory);
    }

    const auto [resultBuffer, deviceMemory] = Buffer::create(
        device,
        physicalDevice,
        bufferSize,
        usage | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    Buffer::copyBuffer(
        device,
        commandPool,
        graphicsQueue,
        stagingBuffer,
        resultBuffer,
        bufferSize
    );

    device.destroyBuffer(stagingBuffer);
    device.freeMemory(stagingBufferMemory);

    return std::make_tuple(resultBuffer, deviceMemory);
}
}  // namespace Buffer

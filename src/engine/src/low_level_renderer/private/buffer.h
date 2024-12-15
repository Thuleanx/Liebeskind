#pragma once

#include <optional>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace Buffer {
std::tuple<vk::Buffer, vk::DeviceMemory> create(
    const vk::Device& device,
    const vk::PhysicalDevice& physicalDevice,
    const vk::DeviceSize size,
    const vk::BufferUsageFlags usage,
    const vk::MemoryPropertyFlags properties
);

std::optional<uint32_t> findSuitableMemoryType(
    const vk::PhysicalDeviceMemoryProperties& memoryProperties,
    const uint32_t typeFilter,
    const vk::MemoryPropertyFlags properties
);

void copyBuffer(
    const vk::Device& device,
    const vk::CommandPool& commandPool,
    const vk::Queue& submitQueue,
    const vk::Buffer srcBuffer,
    const vk::Buffer dstBuffer,
    const vk::DeviceSize size
);

template <typename T>
std::tuple<vk::Buffer, vk::DeviceMemory> loadToBuffer(
    const vk::Device& device,
    const vk::PhysicalDevice& physicalDevice,
    const vk::CommandPool& commandPool,
    const vk::Queue& graphicsQueue,
    const std::vector<T>& data
);
}  // namespace Buffer

#include "buffer_templated.cpp"

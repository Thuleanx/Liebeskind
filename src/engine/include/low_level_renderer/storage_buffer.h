#pragma once

#include <vulkan/vulkan.hpp>
#include <span>

template <typename T>
struct StorageBuffer {
    vk::Buffer buffer;
    vk::DeviceMemory memory;
    void* mappedMemory;
    uint16_t numberOfEntries;

   public:
    static StorageBuffer create(
        const vk::Device& device,
        const vk::PhysicalDevice& physicalDevice,
        uint16_t numberOfEntries
    );

    vk::DescriptorBufferInfo getDescriptorBufferInfo() const;
    
    void update(const std::span<T>& data);

    void destroyBy(const vk::Device& device) const;
};

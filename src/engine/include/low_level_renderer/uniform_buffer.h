#pragma once

#include <vulkan/vulkan.hpp>

template <typename T>
struct UniformBuffer {
    vk::Buffer buffer;
    vk::DeviceMemory memory;
    void* mappedMemory;

   public:
    static UniformBuffer create(
        const vk::Device& device, const vk::PhysicalDevice& physicalDevice
    );
    vk::DescriptorBufferInfo getDescriptorBufferInfo() const;
    void update(const T& data);
    void destroyBy(const vk::Device& device) const;

    UniformBuffer() = default;

   private:
    UniformBuffer(
        vk::Buffer buffer, vk::DeviceMemory deviceMemory, void* mappedMemory
    ) :
        buffer(buffer), memory(deviceMemory), mappedMemory(mappedMemory) {}
};

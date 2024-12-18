#pragma once

#include <vulkan/vulkan.hpp>

template <typename T>
class UniformBuffer {
   public:
    UniformBuffer() = delete;

    static UniformBuffer create(
        const vk::Device& device, const vk::PhysicalDevice& physicalDevice
    );
    vk::DescriptorBufferInfo getDescriptorBufferInfo() const;
    void update(const T& data);
    void destroyBy(const vk::Device& device);
   private:
    UniformBuffer(
        vk::Buffer buffer, vk::DeviceMemory deviceMemory, void* mappedMemory
    );

   private:
    vk::Buffer buffer;
    vk::DeviceMemory memory;
    void* mappedMemory;
};

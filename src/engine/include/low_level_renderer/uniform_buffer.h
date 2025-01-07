#pragma once

#include <vulkan/vulkan.hpp>

template <typename T>
struct UniformBuffer {
    static UniformBuffer create(
        const vk::Device& device, const vk::PhysicalDevice& physicalDevice
    );
    vk::DescriptorBufferInfo getDescriptorBufferInfo() const;
    void update(const T& data);
    void destroyBy(const vk::Device& device);

    UniformBuffer() = default;

    vk::Buffer buffer;
    vk::DeviceMemory memory;
    void* mappedMemory;

   private:
    UniformBuffer(
        vk::Buffer buffer, vk::DeviceMemory deviceMemory, void* mappedMemory
    );

    friend class GraphicsDeviceInterface;
};

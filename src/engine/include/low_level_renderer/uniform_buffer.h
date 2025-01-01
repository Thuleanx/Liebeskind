#pragma once

#include <vulkan/vulkan.hpp>

template <typename T>
class UniformBuffer {
   public:

    static UniformBuffer create(
        const vk::Device& device, const vk::PhysicalDevice& physicalDevice
    );
    vk::DescriptorBufferInfo getDescriptorBufferInfo() const;
    void update(const T& data);
    void destroyBy(const vk::Device& device);

    inline vk::Buffer getBuffer() const { return buffer; }
    UniformBuffer() = default;
   private:
    UniformBuffer(
        vk::Buffer buffer, vk::DeviceMemory deviceMemory, void* mappedMemory
    );

   private:
    vk::Buffer buffer;
    vk::DeviceMemory memory;
    void* mappedMemory;

    friend class GraphicsDeviceInterface;
};

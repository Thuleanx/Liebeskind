#pragma once

#include <vulkan/vulkan.hpp>

template <typename T>
class UniformBuffer {
   public:
    UniformBuffer() = delete;

    UniformBuffer create(const vk::Device& device);
   private:
   private:
    vk::Buffer buffer;
    vk::DeviceMemory memory;
    void* mappedMemory;
};

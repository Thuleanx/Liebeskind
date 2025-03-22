#pragma once

#include <vulkan/vulkan.hpp>

#include "low_level_renderer/descriptor_write_buffer.h"

namespace graphics {
enum class DataBufferType {
    UNIFORM,  // smaller buffer meant for smaller data
    STORAGE   // larger buffer meant for large arrays of data
};

template <typename T, DataBufferType bufferType>
struct DataBuffer {
    vk::Buffer buffer;
    vk::DeviceMemory memory;
    void* mappedMemory;
    uint16_t dataCount;

   public:
    static DataBuffer create(
        const vk::Device& device,
        const vk::PhysicalDevice& physicalDevice,
        uint16_t dataCount = 1
    );
    void bind(
        DescriptorWriteBuffer& writeBuffer, vk::DescriptorSet set, int binding
    ) const;
    void update(const T& data) const;
    void update(std::span<const T> data) const;
    void destroyBy(const vk::Device& device) const;
};

template <typename T>
using UniformBuffer = DataBuffer<T, DataBufferType::UNIFORM>;

template <typename T>
using StorageBuffer = DataBuffer<T, DataBufferType::STORAGE>;
}  // namespace Graphics

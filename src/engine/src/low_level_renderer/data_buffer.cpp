#include "low_level_renderer/data_buffer.h"

#include "private/buffer.h"

namespace graphics {
template <typename T, DataBufferType E>
DataBuffer<T, E> DataBuffer<T, E>::create(
    const vk::Device& device,
    const vk::PhysicalDevice& physicalDevice,
    uint16_t dataCount
) {
    vk::DeviceSize bufferSize = sizeof(T) * dataCount;

    auto [buffer, memory] = Buffer::create(
        device,
        physicalDevice,
        bufferSize,
        E == DataBufferType::UNIFORM ? vk::BufferUsageFlagBits::eUniformBuffer
                                     : vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent
    );

    const vk::ResultValue<void*> mappedMemory =
        device.mapMemory(memory, 0, bufferSize, {});
    VULKAN_ENSURE_SUCCESS(mappedMemory.result, "Can't map buffer memory");

    return DataBuffer{buffer, memory, mappedMemory.value, dataCount};
}

template <typename T, DataBufferType E>
void DataBuffer<T, E>::bind(
    DescriptorWriteBuffer& writeBuffer, vk::DescriptorSet set, int binding
) const {
    writeBuffer.writeBuffer(
        set,
        binding,
        buffer,
        E == DataBufferType::UNIFORM ? vk::DescriptorType::eUniformBuffer
                                     : vk::DescriptorType::eStorageBuffer,
        0,
        sizeof(T) * this->dataCount
    );
}

template <typename T, DataBufferType E>
void DataBuffer<T, E>::update(const T& data) const {
    static size_t bufferSize = sizeof(T);
    memcpy(mappedMemory, &data, bufferSize);
}

template <typename T, DataBufferType E>
void DataBuffer<T, E>::update(std::span<const T> data) const {
    size_t bufferSize = sizeof(T) * data.size();
    memcpy(mappedMemory, data.data(), bufferSize);
}

template <typename T, DataBufferType E>
void DataBuffer<T, E>::destroyBy(const vk::Device& device) const {
    device.unmapMemory(memory);
    device.destroyBuffer(buffer);
    device.freeMemory(memory);
}

}  // namespace Graphics


#include <glm/glm.hpp>
template struct graphics::DataBuffer<glm::mat4, graphics::DataBufferType::UNIFORM>;

#include "low_level_renderer/shader_data.h"
template struct graphics::DataBuffer<graphics::GPUSceneData, graphics::DataBufferType::UNIFORM>;

#include "low_level_renderer/materials.h"
template struct graphics::DataBuffer<graphics::MaterialProperties, graphics::DataBufferType::UNIFORM>;

#include "low_level_renderer/instance_rendering.h"
template struct graphics::DataBuffer<graphics::InstanceData, graphics::DataBufferType::STORAGE>;

#include "low_level_renderer/bloom.h"
template struct graphics::DataBuffer<graphics::BloomSharedBuffer, graphics::DataBufferType::UNIFORM>;
template struct graphics::DataBuffer<graphics::BloomUpsampleBuffer, graphics::DataBufferType::UNIFORM>;
template struct graphics::DataBuffer<graphics::BloomCombineBuffer, graphics::DataBufferType::UNIFORM>;

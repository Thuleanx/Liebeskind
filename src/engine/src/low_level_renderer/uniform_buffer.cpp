#include "low_level_renderer/uniform_buffer.h"

#include "low_level_renderer/material_manager.h"
#include "low_level_renderer/shader_data.h"
#include "private/buffer.h"

template <typename T>
UniformBuffer<T> UniformBuffer<T>::create(
    const vk::Device& device, const vk::PhysicalDevice& physicalDevice
) {
    static vk::DeviceSize bufferSize = sizeof(T);

    auto [buffer, memory] = Buffer::create(
        device,
        physicalDevice,
        bufferSize,
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent
    );

    const vk::ResultValue<void*> mappedMemory =
        device.mapMemory(memory, 0, bufferSize, {});
    VULKAN_ENSURE_SUCCESS(
        mappedMemory.result, "Can't map staging buffer memory"
    );

    return UniformBuffer(buffer, memory, mappedMemory.value);
}

template <typename T>
UniformBuffer<T>::UniformBuffer(
    vk::Buffer buffer, vk::DeviceMemory deviceMemory, void* mappedMemory
) : buffer(buffer), memory(deviceMemory), mappedMemory(mappedMemory) {}

template <typename T>
vk::DescriptorBufferInfo UniformBuffer<T>::getDescriptorBufferInfo() const {
    return vk::DescriptorBufferInfo(
        buffer,
        0,
        sizeof(T)
    );
}

template <typename T>
void UniformBuffer<T>::update(const T& data) {
    static vk::DeviceSize bufferSize = sizeof(T);
    memcpy(mappedMemory, &data, static_cast<size_t>(bufferSize));
}

template <typename T>
void UniformBuffer<T>::destroyBy(const vk::Device& device) {
    device.unmapMemory(memory);
    device.destroyBuffer(buffer);
    device.freeMemory(memory);
}

template struct UniformBuffer<GPUSceneData>;
template struct UniformBuffer<MaterialProperties>;

#include "low_level_renderer/storage_buffer.h"

#include "private/buffer.h"

template <typename T>
StorageBuffer<T> StorageBuffer<T>::create(
    const vk::Device& device,
    const vk::PhysicalDevice& physicalDevice,
    uint16_t numberOfEntries
) {
    vk::DeviceSize bufferSize = sizeof(T) * numberOfEntries;

    auto [buffer, memory] = Buffer::create(
        device,
        physicalDevice,
        bufferSize,
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent
    );

    const vk::ResultValue<void*> mappedMemory =
        device.mapMemory(memory, 0, bufferSize, {});
    VULKAN_ENSURE_SUCCESS(
        mappedMemory.result, "Can't map staging buffer memory"
    );

    return StorageBuffer{buffer, memory, mappedMemory.value};
}

template <typename T>
vk::DescriptorBufferInfo StorageBuffer<T>::getDescriptorBufferInfo() const {
    return vk::DescriptorBufferInfo(buffer, 0, sizeof(T) * numberOfEntries);
}

template <typename T>
void StorageBuffer<T>::update(const std::span<T>& data) {
    static vk::DeviceSize bufferSize = sizeof(T) * data.size();
    memcpy(mappedMemory, &data, static_cast<size_t>(bufferSize));
}

template <typename T>
void StorageBuffer<T>::destroyBy(const vk::Device& device) const {
    device.unmapMemory(memory);
    device.destroyBuffer(buffer);
    device.freeMemory(memory);
}

#include <glm/glm.hpp>
template struct StorageBuffer<glm::mat4>;

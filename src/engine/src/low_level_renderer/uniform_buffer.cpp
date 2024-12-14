#include "low_level_renderer/uniform_buffer.h"

template<typename T>
UniformBuffer<T> UniformBuffer<T>::create(const vk::Device& device) {
    static vk::DeviceSize bufferSize = sizeof(T);
}

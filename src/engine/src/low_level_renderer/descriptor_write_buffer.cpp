#include "low_level_renderer/descriptor_write_buffer.h"

namespace graphics {
void DescriptorWriteBuffer::writeBuffer(
    vk::DescriptorSet descriptorSet,
    int binding,
    const vk::Buffer& buffer,
    vk::DescriptorType type,
    size_t offset,
    size_t range
) {
    buffers.emplace_back(buffer, offset, range);

    const vk::WriteDescriptorSet write(
        descriptorSet, binding, 0, 1, type, nullptr, &buffers.back()
    );
    writes.push_back(std::move(write));
}

void DescriptorWriteBuffer::writeImage(
    vk::DescriptorSet descriptorSet,
    int binding,
    const vk::ImageView& imageView,
    vk::DescriptorType type,
    const vk::Sampler& sampler,
    vk::ImageLayout layout
) {
    images.emplace_back(sampler, imageView, layout);
    const vk::WriteDescriptorSet write(
        descriptorSet, binding, 0, 1, type, &images.back()
    );
    writes.push_back(std::move(write));
}

void DescriptorWriteBuffer::batchWrite(const vk::Device& device) {
    device.updateDescriptorSets(
        static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr
    );
    clear();
}

void DescriptorWriteBuffer::clear() {
    writes.clear();
    buffers.clear();
    images.clear();
}
}  // namespace Graphics

#include "low_level_renderer/descriptor_write_buffer.h"

void DescriptorWriteBuffer::writeBuffer(
    int binding,
    const vk::Buffer& buffer,
    vk::DescriptorType type,
    size_t offset,
    size_t range
) {
    buffers.emplace_back(buffer, offset, range);

    const vk::WriteDescriptorSet write(
        {/* empty descriptor set, to be filled when writing*/},
        binding,
        0,
        1,
        type,
        nullptr,
        &buffers.back()
    );
    writes.push_back(std::move(write));
}

void DescriptorWriteBuffer::writeImage(
    int binding,
    const vk::ImageView& imageView,
    vk::DescriptorType type,
    const vk::Sampler& sampler,
    vk::ImageLayout layout
) {
    images.emplace_back(sampler, imageView, layout);
    const vk::WriteDescriptorSet write(
        {/* empty descriptor set, to be filled when writing*/},
        binding,
        0,
        1,
        type,
        &images.back()
    );
    writes.push_back(std::move(write));
}

void DescriptorWriteBuffer::batch_write(
    const vk::Device& device, const vk::DescriptorSet& descriptorSet
) {
    for (auto& write : writes) write.setDstSet(descriptorSet);
    device.updateDescriptorSets(
        static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr
    );
}

void DescriptorWriteBuffer::clear() {
    writes.clear();
    buffers.clear();
    images.clear();
}

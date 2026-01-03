#include "low_level_renderer/descriptor_write_buffer.h"
#include "core/logger/assert.h"
#include "core/logger/logger.h"

#include <cpptrace/cpptrace.hpp>

namespace graphics {
void DescriptorWriteBuffer::writeBuffer(
    vk::DescriptorSet descriptorSet,
    int binding,
    const vk::Buffer& buffer,
    vk::DescriptorType type,
    size_t offset,
    size_t range
) {
    ASSERT(numberOfBuffersInfo < MAX_DESCRIPTOR_WRITES, "Flush write buffer! It's exceeding its capacity");

    const vk::WriteDescriptorSet write(
        descriptorSet, binding, 0, 1, type, nullptr, &buffers[numberOfBuffersInfo]
    );
    buffers[numberOfBuffersInfo++] = vk::DescriptorBufferInfo(buffer, offset, range);
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
    ASSERT(numberOfImagesInfo < MAX_DESCRIPTOR_WRITES, "Flush write buffer! It's exceeding its capacity");

    const vk::WriteDescriptorSet write(
        descriptorSet, binding, 0, 1, type, &images[numberOfImagesInfo]
    );
    images[numberOfImagesInfo++] = vk::DescriptorImageInfo(sampler, imageView, layout);
    writes.push_back(write);
}

void DescriptorWriteBuffer::batchWrite(const vk::Device& device) {
    LLOG_INFO << "Write: " << cpptrace::generate_trace().to_string(true);
    device.updateDescriptorSets(
        static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr
    );
    clear();
}

void DescriptorWriteBuffer::clear() {
    writes.clear();
    numberOfImagesInfo = numberOfBuffersInfo = 0;
}
}  // namespace Graphics

#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

namespace graphics {

const int MAX_DESCRIPTOR_WRITES = 100;

struct DescriptorWriteBuffer {
    int numberOfBuffersInfo = 0;
    int numberOfImagesInfo = 0;
    std::array<vk::DescriptorBufferInfo, MAX_DESCRIPTOR_WRITES> buffers;
    std::array<vk::DescriptorImageInfo, MAX_DESCRIPTOR_WRITES> images;
    std::vector<vk::WriteDescriptorSet> writes;

   public:
    void writeBuffer(
        vk::DescriptorSet descriptorSet,
        int binding,
        const vk::Buffer& buffer,
        vk::DescriptorType type,
        size_t offset,
        size_t range
    );

    void writeImage(
        vk::DescriptorSet descriptorSet,
        int binding,
        const vk::ImageView& imageView,
        vk::DescriptorType type,
        const vk::Sampler& sampler,
        vk::ImageLayout layout
    );

    void flush(const vk::Device& device);
    void clear();
};
}  // namespace Graphics

#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

struct DescriptorWriteBuffer {
    std::vector<vk::DescriptorBufferInfo> buffers;
    std::vector<vk::DescriptorImageInfo> images;
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

    void batchWrite(
        const vk::Device& device
    );
    void clear();
};

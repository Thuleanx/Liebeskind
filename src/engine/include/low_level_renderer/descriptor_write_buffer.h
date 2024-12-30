#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

class DescriptorWriteBuffer {
   public:
    void writeBuffer(
        int binding,
        const vk::Buffer& buffer,
        vk::DescriptorType type,
        size_t offset,
        size_t range
    );

    void writeImage(
        int binding,
        const vk::ImageView& imageView,
        vk::DescriptorType type,
        const vk::Sampler& sampler,
        vk::ImageLayout layout
    );

    void batch_write(
        const vk::Device& device, const vk::DescriptorSet& descriptorSet
    );
    void clear();

   private:
    std::vector<vk::DescriptorBufferInfo> buffers;
    std::vector<vk::DescriptorImageInfo> images;
    std::vector<vk::WriteDescriptorSet> writes;
};

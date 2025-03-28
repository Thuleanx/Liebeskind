#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

namespace graphics {
struct DescriptorAllocator {
    std::vector<vk::DescriptorPoolSize> poolSizes;
    std::vector<vk::DescriptorPool> readyPools;
    std::vector<vk::DescriptorPool> fullPools;
    uint32_t setsPerPool;

   public:
    [[nodiscard]]
    static DescriptorAllocator create(
        const vk::Device& device,
        std::span<const vk::DescriptorPoolSize> poolSizes,
        uint32_t setsPerPool
    );

    [[nodiscard]]
    std::vector<vk::DescriptorSet> allocate(
        const vk::Device& device,
        const vk::DescriptorSetLayout& setLayout,
        uint32_t numberOfSets
    );
    void clearPools(const vk::Device& device);
    void destroyBy(const vk::Device& device) const;

   private:
    vk::DescriptorPool createPool(const vk::Device& device) const;
};
}  // namespace graphics

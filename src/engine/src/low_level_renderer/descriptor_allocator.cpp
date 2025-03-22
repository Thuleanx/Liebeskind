#include "low_level_renderer/descriptor_allocator.h"

#include "core/logger/vulkan_ensures.h"

namespace {
constexpr uint32_t MAX_DESCRIPTOR_SETS_PER_POOL = 4096;
}

namespace graphics {
DescriptorAllocator DescriptorAllocator::create(
    [[maybe_unused]] const vk::Device& device,
    const std::vector<vk::DescriptorPoolSize>& poolSizes,
    uint32_t setsPerPool
) {
    std::vector<vk::DescriptorPoolSize> allocatorPoolSizes;
    allocatorPoolSizes.reserve(poolSizes.size());
    for (const vk::DescriptorPoolSize& poolSize : poolSizes)
        allocatorPoolSizes.emplace_back(
            poolSize.type, poolSize.descriptorCount
        );
    return DescriptorAllocator{allocatorPoolSizes, {}, {}, setsPerPool};
}

vk::DescriptorPool DescriptorAllocator::createPool(const vk::Device& device
) const {
    std::vector<vk::DescriptorPoolSize> instantiatedPoolSizes(poolSizes);
    for (size_t i = 0; i < instantiatedPoolSizes.size(); i++)
        instantiatedPoolSizes[i].descriptorCount *= setsPerPool;

    const vk::DescriptorPoolCreateInfo poolInfo(
        {},
        setsPerPool,
        static_cast<uint32_t>(instantiatedPoolSizes.size()),
        instantiatedPoolSizes.data()
    );

    const vk::ResultValue<vk::DescriptorPool> descriptorPoolCreation =
        device.createDescriptorPool(poolInfo);
    VULKAN_ENSURE_SUCCESS(
        descriptorPoolCreation.result, "Can't create descriptor pool:"
    );
    return descriptorPoolCreation.value;
}

std::vector<vk::DescriptorSet> DescriptorAllocator::allocate(
    const vk::Device& device,
    const vk::DescriptorSetLayout& setLayout,
    uint32_t numberOfSets
) {
    ASSERT(
        numberOfSets <= MAX_DESCRIPTOR_SETS_PER_POOL,
        "Number of descriptor sets requested at once "
            << numberOfSets << " is greater than the maximum number allowed "
            << MAX_DESCRIPTOR_SETS_PER_POOL
    );
    const std::vector<vk::DescriptorSetLayout> setLayouts(
        numberOfSets, setLayout
    );
    while (1) {
        if (!readyPools.size()) readyPools.push_back(createPool(device));
        const vk::DescriptorPool pool = readyPools.back();
        readyPools.pop_back();
        const vk::DescriptorSetAllocateInfo allocateInfo(
            pool, numberOfSets, setLayouts.data()
        );
        const vk::ResultValue<std::vector<vk::DescriptorSet>>
            descriptorSetCreation = device.allocateDescriptorSets(allocateInfo);
        switch (descriptorSetCreation.result) {
            case vk::Result::eErrorOutOfPoolMemory:
            case vk::Result::eErrorFragmentedPool:
                fullPools.push_back(pool);
                break;
            default:
                VULKAN_ENSURE_SUCCESS(
                    descriptorSetCreation.result,
                    "Unexpected error when creating descriptor sets"
                );
                readyPools.push_back(pool);
                return descriptorSetCreation.value;
        }
    }
}

void DescriptorAllocator::clearPools(const vk::Device& device) {
    readyPools.reserve(fullPools.size() + readyPools.size());
    for (const vk::DescriptorPool& pool : readyPools)
        device.resetDescriptorPool(pool);
    for (const vk::DescriptorPool& pool : fullPools) {
        device.resetDescriptorPool(pool);
        readyPools.push_back(pool);
    }
    fullPools.clear();
}

void DescriptorAllocator::destroyBy(const vk::Device& device) const {
    for (const vk::DescriptorPool& pool : readyPools)
        device.destroyDescriptorPool(pool);
    for (const vk::DescriptorPool& pool : fullPools)
        device.destroyDescriptorPool(pool);
}
}  // namespace Graphics

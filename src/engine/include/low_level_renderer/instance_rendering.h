#pragma once

#include <array>
#include <glm/glm.hpp>

#include "low_level_renderer/config.h"
#include "low_level_renderer/descriptor_allocator.h"
#include "low_level_renderer/descriptor_write_buffer.h"
#include "low_level_renderer/data_buffer.h"

struct InstanceData {
    alignas(16) glm::mat4 transform;
};

struct RenderInstance {
    StorageBuffer<InstanceData> storageBuffer;
    vk::DescriptorSet descriptorSet;
};

struct RenderInstanceID {
    uint16_t index;

    bool operator==(const RenderInstanceID& other) const;
};

struct RenderInstanceIDHashFunction {
    size_t operator()(const RenderInstanceID& pos) const;
};

struct RenderInstanceManager {
    std::vector<std::array<RenderInstance, MAX_FRAMES_IN_FLIGHT>>
        renderInstances;

   public:
    [[nodiscard]]
    RenderInstanceID create(
        vk::Device device,
        vk::PhysicalDevice physicalDevice,
        vk::DescriptorSetLayout setLayout,
        DescriptorAllocator& descriptorAllocator,
        DescriptorWriteBuffer& writeBuffer,
        uint16_t numberOfEntries
    );

    void bind(
        vk::CommandBuffer commandBuffer,
        vk::PipelineLayout layout,
        RenderInstanceID id,
        uint32_t currentFrame
    ) const;

    void update(
        RenderInstanceID id,
        uint32_t currentFrame,
        std::span<const InstanceData> instanceData
    ) const;

    uint16_t getCapacity(RenderInstanceID id) const;

    void destroyBy(const vk::Device& device) const;
};

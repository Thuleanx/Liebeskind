#pragma once

#include <vulkan/vulkan.hpp>

#include "low_level_renderer/descriptor_allocator.h"

enum class PipelineType : uint8_t { REGULAR, INSTANCED };

enum class PipelineSetType : uint8_t {
    GLOBAL = 0,
    MATERIAL = 1,
    INSTANCE_RENDERING = 2,
    MAX = INSTANCE_RENDERING,
};

struct MaterialPipeline {
    vk::Pipeline pipeline;
    vk::PipelineLayout layout;
    std::array<
        vk::DescriptorSetLayout,
        static_cast<size_t>(PipelineSetType::MAX) + 1>
        descriptorSetLayouts;
    std::array<
        DescriptorAllocator,
        static_cast<size_t>(PipelineSetType::MAX) + 1>
        descriptorAllocators;
    PipelineType pipelineType;

   public:
    static MaterialPipeline create(
        PipelineType pipelineType,
        vk::Device device,
        vk::ShaderModule vertexShader,
        vk::ShaderModule fragmentShader,
        vk::RenderPass renderPass
    );
    void destroyBy(vk::Device device) const;
};

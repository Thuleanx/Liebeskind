#pragma once

#include <vulkan/vulkan.hpp>

#include "low_level_renderer/descriptor_allocator.h"

struct MaterialPipeline {
    vk::Pipeline pipeline;
    vk::PipelineLayout layout;
    vk::DescriptorSetLayout globalDescriptorSetLayout;
    vk::DescriptorSetLayout materialDescriptorSetLayout;
    DescriptorAllocator globalDescriptorAllocator;
    DescriptorAllocator materialDescriptorAllocator;

   public:
    static MaterialPipeline create(
        vk::Device device,
        vk::ShaderModule vertexShader,
        vk::ShaderModule fragmentShader,
        vk::RenderPass renderPass
    );
    void destroyBy(vk::Device device) const;
};

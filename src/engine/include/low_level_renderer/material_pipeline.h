#pragma once

#include <vulkan/vulkan.hpp>

#include "low_level_renderer/descriptor_allocator.h"

enum class PipelineDescriptorSetBindingPoint {
    eGlobal = 0,
    eMaterial = 1,
    eInstanceRendering = 2,
};

struct PipelineData {
    vk::Pipeline pipeline;
    vk::PipelineLayout layout;
};

struct PipelineDescriptorData {
    vk::DescriptorSetLayout setLayout;
    DescriptorAllocator allocator;
};

struct MaterialPipeline {
    PipelineData regularPipeline;
    PipelineData instanceRenderingPipeline;
    PipelineDescriptorData globalDescriptor;
    PipelineDescriptorData instanceRenderingDescriptor;
    PipelineDescriptorData materialDescriptor;

   public:
    static MaterialPipeline create(
        vk::Device device,
        vk::ShaderModule vertexShader,
        vk::ShaderModule instanceRenderingVertexShader,
        vk::ShaderModule fragmentShader,
        vk::RenderPass renderPass
    );
    void destroyBy(vk::Device device) const;
};

void destroy(const PipelineData& pipelineData, vk::Device device);
void destroy(
    const PipelineDescriptorData& pipelineDescriptor, vk::Device device
);

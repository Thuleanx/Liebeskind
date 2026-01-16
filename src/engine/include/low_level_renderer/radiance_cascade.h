#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include "low_level_renderer/data_buffer.h"
#include "low_level_renderer/shaders.h"

#include "texture.h"

namespace graphics {

struct RadianceCascadeData {
    UniformBuffer<glm::mat4> sceneDataBuffer;
    glm::vec3 center;
    glm::vec3 size;

    uint32_t resolution;

    std::array<Texture, 2> sdfTextures;

    struct Rasterization {
        ShaderID vertex;
        ShaderID geometry;
        ShaderID fragment;

        vk::DescriptorSetLayout setLayout;
        vk::DescriptorSet set;
        vk::DescriptorPool pool;
        vk::PipelineLayout pipelineLayout;
        vk::Pipeline pipeline;
        vk::Framebuffer framebuffer;
        vk::RenderPass renderPass;
    } rasterization;

    struct JumpFlood {
        ShaderID compute;

        vk::DescriptorSetLayout setLayout;
        vk::DescriptorPool pool;
        std::array<vk::DescriptorSet, 2> textureSets;

        vk::PipelineLayout pipelineLayout;
        std::vector<vk::Pipeline> pipelines;
    } jumpFlood;
};

}

#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include "low_level_renderer/shaders.h"

#include "texture.h"

namespace graphics {

struct RadianceCascadeData {
    glm::vec3 center;
    glm::vec3 size;

    struct SDFData {
        ShaderID vertexShader;
        ShaderID geometryShader;
        ShaderID fragmentShader;

        vk::DescriptorSetLayout globalSetLayout;
        vk::DescriptorPool globalSetPool;
        vk::DescriptorSet globalSet;
        vk::PipelineLayout pipelineLayout;
        vk::Pipeline pipeline;

        vk::RenderPass renderPass;
        Texture texture;
    } sdf;
};

}

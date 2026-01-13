#pragma once

#include "low_level_renderer/radiance_cascade.h"

namespace graphics {

struct RadianceCascadeCreateInfo {
    const vk::Device& device;
    const vk::PhysicalDevice& physicalDevice;
	ShaderStorage& shaders;
    uint32_t resolution;
    glm::vec3 center;
    glm::vec3 size;
};
RadianceCascadeData create(const RadianceCascadeCreateInfo& info);

void destroy(const RadianceCascadeData& radianceCascade, vk::Device device);

}

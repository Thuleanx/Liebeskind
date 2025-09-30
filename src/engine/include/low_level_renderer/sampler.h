#pragma once

#include <vulkan/vulkan.hpp>

namespace graphics {
enum class SamplerType { eLinear, ePoint };

struct Samplers {
    vk::Sampler linear;
    vk::Sampler linearClearBorder;
    vk::Sampler point;

   public:
    static Samplers create(
        const vk::Device& device, const vk::PhysicalDevice& physicalDevice
    );
};

void destroy(vk::Device device, const Samplers& samplers);
}  // namespace Graphics

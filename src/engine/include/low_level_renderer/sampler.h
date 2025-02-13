#pragma once

#include <vulkan/vulkan.hpp>

struct Sampler {
    vk::Sampler sampler;

   public:
    [[nodiscard]]
    static Sampler create(
        const vk::Device& device, const vk::PhysicalDevice& physicalDevice
    );
    void destroyBy(const vk::Device& device) const;

   private:
    Sampler(vk::Sampler sampler);
};

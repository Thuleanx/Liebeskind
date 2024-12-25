#pragma once

#include <vulkan/vulkan.hpp>

class Sampler {
   public:
    [[nodiscard]]
    static Sampler create(
        const vk::Device& device, const vk::PhysicalDevice& physicalDevice
    );

    void destroyBy(const vk::Device& device);

   private:
    Sampler(vk::Sampler sampler);

   private:
    vk::Sampler sampler;
};

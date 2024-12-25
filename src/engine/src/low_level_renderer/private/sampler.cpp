#include "low_level_renderer/sampler.h"

#include "helpful_defines.h"

Sampler Sampler::create(
    const vk::Device &device, const vk::PhysicalDevice &physicalDevice
) {
    const vk::PhysicalDeviceProperties properties =
        physicalDevice.getProperties();

    const vk::SamplerCreateInfo samplerInfo(
        {},
        vk::Filter::eLinear,
        vk::Filter::eLinear,
        vk::SamplerMipmapMode::eLinear,
        vk::SamplerAddressMode::eRepeat,
        vk::SamplerAddressMode::eRepeat,
        vk::SamplerAddressMode::eRepeat,
        0,         // mip lod bias
        vk::True,  // enable anisotropy
        properties.limits.maxSamplerAnisotropy,
        vk::False,  // compare enable
        vk::CompareOp::eAlways,
        0,
        0,
        vk::BorderColor::eIntOpaqueBlack,
        vk::False
    );

    const vk::ResultValue<vk::Sampler> samplerCreation =
        device.createSampler(samplerInfo);

    VULKAN_ENSURE_SUCCESS(samplerCreation.result, "Can't create sampler");

    return Sampler(samplerCreation.value);
}

vk::Sampler Sampler::getSampler() const {
    return sampler;
}

void Sampler::destroyBy(const vk::Device &device) {
    device.destroySampler(sampler);
}

Sampler::Sampler(vk::Sampler sampler) : sampler(sampler) {}

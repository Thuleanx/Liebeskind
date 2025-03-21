#include "low_level_renderer/sampler.h"

#include "core/logger/vulkan_ensures.h"

Graphics::Samplers Graphics::Samplers::create(
    const vk::Device &device, const vk::PhysicalDevice &physicalDevice
) {
    const vk::PhysicalDeviceProperties properties =
        physicalDevice.getProperties();
    const vk::SamplerCreateInfo linearSamplerInfo(
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
        VK_LOD_CLAMP_NONE,
        vk::BorderColor::eIntOpaqueBlack,
        vk::False
    );
    const vk::SamplerCreateInfo pointSamplerInfo(
        {},
        vk::Filter::eNearest,
        vk::Filter::eNearest,
        vk::SamplerMipmapMode::eNearest,
        vk::SamplerAddressMode::eRepeat,
        vk::SamplerAddressMode::eRepeat,
        vk::SamplerAddressMode::eRepeat,
        0,         // mip lod bias
        vk::True,  // enable anisotropy
        properties.limits.maxSamplerAnisotropy,
        vk::False,  // compare enable
        vk::CompareOp::eAlways,
        0,
        VK_LOD_CLAMP_NONE,
        vk::BorderColor::eIntOpaqueBlack,
        vk::False
    );
    const vk::ResultValue<vk::Sampler> linearSamplerCreation =
        device.createSampler(linearSamplerInfo);

    VULKAN_ENSURE_SUCCESS(
        linearSamplerCreation.result, "Can't create linear sampler"
    );

    const vk::ResultValue<vk::Sampler> pointSamplerCreation =
        device.createSampler(pointSamplerInfo);

    VULKAN_ENSURE_SUCCESS(
        pointSamplerCreation.result, "Can't create point sampler"
    );

    return Samplers {
        .linear = linearSamplerCreation.value,
        .point = pointSamplerCreation.value
    };
}

void Graphics::destroy(vk::Device device, const Samplers& samplers) {
    device.destroySampler(samplers.linear);
    device.destroySampler(samplers.point);
}

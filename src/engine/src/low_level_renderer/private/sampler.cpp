#include "low_level_renderer/sampler.h"

#include "core/logger/vulkan_ensures.h"

namespace graphics {
Samplers Samplers::create(
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
        vk::BorderColor::eFloatTransparentBlack,
        vk::False
    );
    const vk::SamplerCreateInfo linearClearBorderSamplerInfo(
        {},
        vk::Filter::eLinear,
        vk::Filter::eLinear,
        vk::SamplerMipmapMode::eLinear,
        vk::SamplerAddressMode::eClampToBorder,
        vk::SamplerAddressMode::eClampToBorder,
        vk::SamplerAddressMode::eClampToBorder,
        0,         // mip lod bias
        vk::True,  // enable anisotropy
        properties.limits.maxSamplerAnisotropy,
        vk::False,  // compare enable
        vk::CompareOp::eAlways,
        0,
        VK_LOD_CLAMP_NONE,
        vk::BorderColor::eFloatTransparentBlack,
        vk::False
    );
    const vk::SamplerCreateInfo pointSamplerInfo(
        {},
        vk::Filter::eNearest,
        vk::Filter::eNearest,
        vk::SamplerMipmapMode::eNearest,
        vk::SamplerAddressMode::eClampToBorder,
        vk::SamplerAddressMode::eClampToBorder,
        vk::SamplerAddressMode::eClampToBorder,
        0,         // mip lod bias
        vk::True,  // enable anisotropy
        properties.limits.maxSamplerAnisotropy,
        vk::False,  // compare enable
        vk::CompareOp::eAlways,
        0,
        VK_LOD_CLAMP_NONE,
        vk::BorderColor::eFloatOpaqueBlack,
        vk::False
    );
    const vk::ResultValue<vk::Sampler> linearSamplerCreation =
        device.createSampler(linearSamplerInfo);

    VULKAN_ENSURE_SUCCESS(
        linearSamplerCreation.result, "Can't create linear sampler"
    );

    const vk::ResultValue<vk::Sampler> linearClearBorderSamplerCreation =
        device.createSampler(linearClearBorderSamplerInfo);

    VULKAN_ENSURE_SUCCESS(
        linearSamplerCreation.result, "Can't create linear clear border sampler"
    );

    const vk::ResultValue<vk::Sampler> pointSamplerCreation =
        device.createSampler(pointSamplerInfo);

    VULKAN_ENSURE_SUCCESS(
        pointSamplerCreation.result, "Can't create point sampler"
    );

    return Samplers{
        .linear = linearSamplerCreation.value,
        .linearClearBorder = linearClearBorderSamplerCreation.value,
        .point = pointSamplerCreation.value
    };
}

void destroy(vk::Device device, const Samplers &samplers) {
    device.destroySampler(samplers.linear);
    device.destroySampler(samplers.linearClearBorder);
    device.destroySampler(samplers.point);
}
}  // namespace graphics

#pragma once

#include "low_level_renderer/bloom.h"
#include "low_level_renderer/graphics_device_interface.h"

namespace graphics {

constexpr std::array<vk::DescriptorSetLayoutBinding, 1> BLOOM_BINDINGS = {
	vk::DescriptorSetLayoutBinding{
		0,	// binding
		vk::DescriptorType::eCombinedImageSampler,
		1,	// descriptor count
		vk::ShaderStageFlagBits::eFragment
	},
};
constexpr std::array<vk::DescriptorPoolSize, 1> BLOOM_DESCRIPTOR_POOL_SIZES = {
	vk::DescriptorPoolSize(
		vk::DescriptorType::eCombinedImageSampler, NUM_BLOOM_LAYERS
	),
};

constexpr std::array<vk::SpecializationMapEntry, 2> BLOOM_SPECIALIZATION_INFO = {
	vk::SpecializationMapEntry{0, 
		offsetof(BloomSpecializationConstants, texelSize),
        sizeof(glm::vec2)},
	vk::SpecializationMapEntry{
		1,
		offsetof(BloomSpecializationConstants, sampleDistance),
		sizeof(float)
	}
};

BloomData createBloomData(
	const GraphicsDeviceInterface& device,
	const BloomSettings& settings,
	vk::Extent2D swapchainExtent,
	uint32_t swapchainSize
);
void destroy(const BloomData& bloomData, vk::Device device);

vk::RenderPass createBloomRenderpass(
	vk::Device device, vk::Format colorAttachmentFormat
);

BloomPipeline createBloomPipeline(
	ShaderStorage& shaders,
	vk::Device device,
	const RenderPassData& renderPasses,
	vk::Extent2D swapchainExtent
);
void destroy(const BloomPipeline& bloomPipeline, vk::Device device);
}  // namespace graphics

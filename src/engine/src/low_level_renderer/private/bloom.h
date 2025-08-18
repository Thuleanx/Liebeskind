#pragma once

#include "low_level_renderer/bloom.h"
#include "low_level_renderer/renderpass_data.h"
#include "low_level_renderer/shaders.h"

namespace graphics {

constexpr std::array<vk::DescriptorSetLayoutBinding, 1> BLOOM_TEXTURE_BINDINGS = {
	vk::DescriptorSetLayoutBinding{
		0,	// binding
		vk::DescriptorType::eCombinedImageSampler,
		1,	// descriptor count
		vk::ShaderStageFlagBits::eFragment
	}
};

constexpr std::array<vk::DescriptorSetLayoutBinding, 1> BLOOM_SWAPCHAIN_BINDINGS = {
	vk::DescriptorSetLayoutBinding{
		0,	// binding
		vk::DescriptorType::eUniformBuffer,
		1,	// descriptor count
		vk::ShaderStageFlagBits::eFragment
	},
};

constexpr std::array<vk::DescriptorPoolSize, 1> BLOOM_TEXTURE_DESCRIPTOR_POOL_SIZES = {
	vk::DescriptorPoolSize(
		vk::DescriptorType::eCombinedImageSampler, 1
	),
};

constexpr std::array<vk::DescriptorPoolSize, 1> BLOOM_SWAPCHAIN_DESCRIPTOR_POOL_SIZES = {
	vk::DescriptorPoolSize(
		vk::DescriptorType::eUniformBuffer, 1
	),
};

constexpr std::array<vk::SpecializationMapEntry, 2> BLOOM_SPECIALIZATION_INFO =
	{vk::SpecializationMapEntry{
		 0,
		 offsetof(BloomSpecializationConstants, sampleDistance),
		 sizeof(float)
	 },
	 vk::SpecializationMapEntry{
		 1, offsetof(BloomSpecializationConstants, texelScale), sizeof(float)
	 }};

BloomSwapchainData createBloomData(
    vk::Device device,
    vk::PhysicalDevice physicalDevice,
	const BloomSettings& settings,
    BloomPipeline& pipeline,
    vk::Format imageFormat,
	vk::Extent2D swapchainExtent,
	size_t swapchainSize
);
void destroy(const BloomSwapchainData& bloomData, vk::Device device);

vk::RenderPass createBloomRenderpass(
	vk::Device device, vk::Format colorAttachmentFormat
);

BloomPipeline createBloomPipeline(
	ShaderStorage& shaders,
	vk::Device device,
	vk::PhysicalDevice physicalDevice,
	const RenderPassData& renderPasses
);
void updateBloomUniform(BloomPipeline& bloomPipeline, vk::Extent2D swapchainExtent);
void destroy(const BloomPipeline& bloomPipeline, vk::Device device);

}  // namespace graphics

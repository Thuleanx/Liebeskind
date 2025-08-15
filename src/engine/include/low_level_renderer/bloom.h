#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

#include "low_level_renderer/descriptor_allocator.h"

namespace graphics {

constexpr uint32_t NUM_BLOOM_LAYERS = 3;

constexpr std::array<vk::DescriptorSetLayoutBinding, 2> BLOOM_BINDINGS = {
	vk::DescriptorSetLayoutBinding{
		0,	// binding
		vk::DescriptorType::eCombinedImageSampler,
		1,	// descriptor count
		vk::ShaderStageFlagBits::eFragment
	},
	vk::DescriptorSetLayoutBinding{
		1,	// binding
		vk::DescriptorType::eUniformBuffer,
		1,	// descriptor count
		vk::ShaderStageFlagBits::eFragment
	},
};
constexpr std::array<vk::DescriptorPoolSize, 2> BLOOM_DESCRIPTOR_POOL_SIZES = {
	vk::DescriptorPoolSize(
		vk::DescriptorType::eCombinedImageSampler, NUM_BLOOM_LAYERS
	),
	vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1),
};

struct BloomPipeline {
    std::array<vk::Pipeline, 2*NUM_BLOOM_LAYERS> pipelines;
	vk::PipelineLayout layout;
	vk::DescriptorSetLayout descriptorLayout;
	DescriptorAllocator allocator;
};

struct BloomSettings {};

struct BloomData {
	struct Attachment {
		vk::Image image;
		vk::DeviceMemory memory;
		std::array<vk::ImageView, NUM_BLOOM_LAYERS> mipViews;
	};
	std::vector<Attachment> colorAttachments;
};

};	// namespace graphics

#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "data_buffer.h"
#include "low_level_renderer/descriptor_allocator.h"

namespace graphics {

constexpr uint32_t NUM_BLOOM_LAYERS = 3;
// NUM_BLOOM_LAYERS downsample passes and NUM_BLOOM_LAYERS upsample passes
constexpr uint32_t NUM_BLOOM_PASSES = NUM_BLOOM_LAYERS * 2;

struct BloomUniformBuffer {
    glm::vec2 swapchainExtent;
};

struct BloomPipeline {
    std::array<vk::Pipeline, NUM_BLOOM_PASSES> pipelines;
	vk::PipelineLayout layout;
	vk::DescriptorSetLayout textureSetLayout;
	vk::DescriptorSetLayout swapchainSetLayout;
    vk::DescriptorPool swapchainDescriptorPool;
    vk::DescriptorSet swapchainDescriptor;
    DescriptorAllocator textureDescriptorAllocator;
    UniformBuffer<BloomUniformBuffer> ubo;
};

struct BloomSpecializationConstants {
    float texelScale;
    float sampleDistance;
};

struct BloomSettings {};

struct BloomSwapchainData {
	struct Attachment {
		vk::Image image;
		vk::DeviceMemory memory;
		std::array<vk::ImageView, NUM_BLOOM_LAYERS> mipViews;
        std::array<vk::DescriptorSet, NUM_BLOOM_PASSES> textureDescriptors;
	};
	std::vector<Attachment> attachments;
};

};	// namespace graphics

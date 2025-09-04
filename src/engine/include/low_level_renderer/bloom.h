#pragma once

#include <glm/glm.hpp>
#include <optional>
#include <vulkan/vulkan.hpp>

#include "data_buffer.h"
#include "low_level_renderer/descriptor_allocator.h"

namespace graphics {

constexpr size_t NUM_BLOOM_LAYERS = 3;
constexpr size_t NUM_BLOOM_MIPS = NUM_BLOOM_LAYERS + 1;
// NUM_BLOOM_LAYERS downsample passes and NUM_BLOOM_LAYERS upsample passes
constexpr size_t NUM_BLOOM_PASSES = NUM_BLOOM_LAYERS * 2;

struct BloomSharedBuffer {
	glm::vec2 baseMipSize;
};

struct BloomUpsampleBuffer {
    float blurRadius;
};

struct BloomCombineBuffer {
    float intensity;
    float blurRadius;
};

struct BloomGraphicsObjects {
	static_assert(NUM_BLOOM_PASSES > 0);

	struct RenderPasses {
		vk::RenderPass downsample;
		vk::RenderPass upsample;
		vk::RenderPass combine;
	} renderPasses;
	struct LayerSetLayouts {
		vk::DescriptorSetLayout downsample;
		vk::DescriptorSetLayout upsample;
		vk::DescriptorSetLayout combine;
		vk::DescriptorSetLayout shared;
	} uniformLayouts;
	struct PipelineLayouts {
		vk::PipelineLayout downsample;
		vk::PipelineLayout upsample;
		vk::PipelineLayout combine;
	} pipelineLayouts;
	struct DescriptorPools {
		DescriptorAllocator layer_downsample;
		DescriptorAllocator layer_upsample;
		DescriptorAllocator layer_combine;
		vk::DescriptorPool shared;
	} pools;
	struct Descriptors {
		vk::DescriptorSet shared;
	} descriptors;
	struct DataBuffers {
		UniformBuffer<BloomUpsampleBuffer> upsample;
		UniformBuffer<BloomCombineBuffer> combine;
		UniformBuffer<BloomSharedBuffer> shared;
	} buffers;
	std::array<vk::Pipeline, NUM_BLOOM_PASSES> pipelines;

	struct SwapchainObject {
		static constexpr size_t NUM_BUFFERS = 2;

        std::array<vk::Image, NUM_BUFFERS> colorBuffer;
        std::array<vk::DeviceMemory, NUM_BUFFERS> colorMemory;
        std::array<std::array<vk::ImageView, NUM_BLOOM_MIPS>, NUM_BUFFERS> colorViews;
		struct Descriptors {
			std::array<vk::DescriptorSet, NUM_BLOOM_LAYERS> downsample;
			std::array<vk::DescriptorSet, NUM_BLOOM_LAYERS - 1> upsample;
			vk::DescriptorSet combine;
		} descriptors;
		std::array<vk::Framebuffer, NUM_BLOOM_PASSES> framebuffers;
	};
	std::optional<std::vector<SwapchainObject>> swapchainObjects;
};

};	// namespace graphics

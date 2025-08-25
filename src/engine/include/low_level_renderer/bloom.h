#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <optional>

#include "data_buffer.h"
#include "low_level_renderer/descriptor_allocator.h"

namespace graphics {

constexpr size_t NUM_BLOOM_LAYERS = 3;
constexpr size_t NUM_BLOOM_MIPS = NUM_BLOOM_LAYERS + 1;
// NUM_BLOOM_LAYERS downsample passes and NUM_BLOOM_LAYERS upsample passes
constexpr size_t NUM_BLOOM_PASSES = NUM_BLOOM_LAYERS * 2;

struct BloomUniformBuffer {
	glm::vec2 swapchainExtent;
};

struct BloomGraphicsObjects {
	struct RenderPasses {
		vk::RenderPass downsample;
		vk::RenderPass upsample;
		vk::RenderPass combine;
	} renderPasses;
	struct SetLayouts {
		vk::DescriptorSetLayout combine;
		vk::DescriptorSetLayout texture;
		vk::DescriptorSetLayout swapchain;
	} setLayouts;
	struct PipelineLayouts {
		vk::PipelineLayout downsample;
		vk::PipelineLayout upsample;
		vk::PipelineLayout combine;
	} pipelineLayouts;
	struct DescriptorPools {
        DescriptorAllocator combine;
        DescriptorAllocator texture;
		vk::DescriptorPool swapchain;
	} pools;
	struct Descriptors {
		static_assert(NUM_BLOOM_PASSES > 0);
		vk::DescriptorSet swapchain;
	} descriptors;
	struct DataBuffers {
		UniformBuffer<BloomUniformBuffer> swapchain;
	} buffers;
	std::array<vk::Pipeline, NUM_BLOOM_PASSES> pipelines;

	struct SwapchainObject {
		vk::Image colorBuffer;
		vk::DeviceMemory colorMemory;
		std::array<vk::ImageView, NUM_BLOOM_MIPS> colorViews;
		struct Descriptors {
			vk::DescriptorSet combine;
			std::array<vk::DescriptorSet, NUM_BLOOM_PASSES - 1> texture;
		} descriptors;
		std::array<vk::Framebuffer, NUM_BLOOM_PASSES> framebuffers;
	};
	std::optional<std::vector<SwapchainObject>> swapchainObjects;
};

struct BloomSpecializationConstants {
	float texelScale;
	float sampleDistance;
};

};	// namespace graphics

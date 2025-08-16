#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "low_level_renderer/descriptor_allocator.h"

namespace graphics {

constexpr uint32_t NUM_BLOOM_LAYERS = 3;

struct BloomPipeline {
    std::array<vk::Pipeline, 2*NUM_BLOOM_LAYERS> pipelines;
	vk::PipelineLayout layout;
	vk::DescriptorSetLayout descriptorLayout;
	DescriptorAllocator allocator;
};

struct BloomSpecializationConstants {
    glm::vec2 texelSize;
    float sampleDistance;
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

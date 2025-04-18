#pragma once

#include <glm/glm.hpp>
#include <array>
#include <vulkan/vulkan.hpp>

namespace graphics {
constexpr std::array<vk::DescriptorSetLayoutBinding, 2>
	POST_PROCESSING_BINDINGS = {
		vk::DescriptorSetLayoutBinding{
			0,	// binding
			vk::DescriptorType::eCombinedImageSampler,
			1,	// descriptor count
			vk::ShaderStageFlagBits::eFragment
		},
		vk::DescriptorSetLayoutBinding{
			1,	// binding
			vk::DescriptorType::eCombinedImageSampler,
			1,	// descriptor count
			vk::ShaderStageFlagBits::eFragment
		},
};
constexpr std::array<vk::DescriptorPoolSize, 1>
	POST_PROCESSING_DESCRIPTOR_POOL_SIZES = {
		vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 2),
};

enum PostProcessingMode {
};

struct PostProcessingData {
    PostProcessingMode mode;
    alignas(4) float exposure;
    float temperature;
    float tint;
    alignas(16) glm::vec3 filter;
};

}  // namespace graphics

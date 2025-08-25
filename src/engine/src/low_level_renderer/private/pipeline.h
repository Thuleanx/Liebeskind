#pragma once

#include <span>
#include <vulkan/vulkan.hpp>

namespace graphics {
vk::PipelineLayout createPipelineLayout(
	vk::Device device,
	std::span<const vk::DescriptorSetLayout> setLayouts,
	std::span<const vk::PushConstantRange> pushConstantRanges
);
}

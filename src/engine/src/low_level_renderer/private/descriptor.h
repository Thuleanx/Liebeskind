#pragma once

#include <span>
#include <vulkan/vulkan.hpp>

namespace graphics {
vk::DescriptorSetLayout createDescriptorLayout(
	vk::Device device, std::span<const vk::DescriptorSetLayoutBinding> bindings
);

vk::DescriptorPool createDescriptorPool(
	vk::Device device,
	uint32_t maxSets,
	std::span<const vk::DescriptorPoolSize> poolSizes
);
};	// namespace graphics

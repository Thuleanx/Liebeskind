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

std::vector<vk::DescriptorSet> createDescriptorSets(
	vk::Device device,
	vk::DescriptorPool pool,
	vk::DescriptorSetLayout layout,
	size_t numberOfSets
);

};	// namespace graphics

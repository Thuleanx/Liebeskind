#include "descriptor.h"

#include "core/logger/assert.h"
#include "core/logger/vulkan_ensures.h"

namespace graphics {
vk::DescriptorSetLayout createDescriptorLayout(
	vk::Device device, std::span<const vk::DescriptorSetLayoutBinding> bindings
) {
	const vk::ResultValue<vk::DescriptorSetLayout> setLayoutCreation =
		device.createDescriptorSetLayout(
			{{}, static_cast<uint32_t>(bindings.size()), bindings.data()}
		);
	VULKAN_ENSURE_SUCCESS(
		setLayoutCreation.result, "Can't create descriptor set layout"
	);
	return setLayoutCreation.value;
}

vk::DescriptorPool createDescriptorPool(
	vk::Device device,
	uint32_t maxSets,
	std::span<const vk::DescriptorPoolSize> poolSizes
) {
	const vk::DescriptorPoolCreateInfo poolInfo(
		{},
		maxSets,  // we only need one descriptor set per pass
		static_cast<uint32_t>(poolSizes.size()),
		poolSizes.data()
	);
	const vk::ResultValue<vk::DescriptorPool> descriptorPoolCreation =
		device.createDescriptorPool(poolInfo);
	VULKAN_ENSURE_SUCCESS(
		descriptorPoolCreation.result, "Can't create desciptor pool:"
	);
	return descriptorPoolCreation.value;
}

std::vector<vk::DescriptorSet> createDescriptorSets(
	vk::Device device,
	vk::DescriptorPool pool,
	vk::DescriptorSetLayout layout,
	size_t numberOfSets
) {
	const std::vector<vk::DescriptorSetLayout> setLayouts(numberOfSets, layout);
	const vk::DescriptorSetAllocateInfo allocateInfo(
		pool, numberOfSets, setLayouts.data()
	);

	const vk::ResultValue<std::vector<vk::DescriptorSet>>
		descriptorSetCreation = device.allocateDescriptorSets(allocateInfo);

	VULKAN_ENSURE_SUCCESS(
		descriptorSetCreation.result,
		"Unexpected error when creating descriptor sets"
	);

	ASSERT(
		descriptorSetCreation.value.size() == numberOfSets,
		"Requested " << numberOfSets << " descriptors sets, received "
					 << descriptorSetCreation.value.size()
	);

	return descriptorSetCreation.value;
}

}  // namespace graphics

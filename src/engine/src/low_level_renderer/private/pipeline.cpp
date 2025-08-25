#include "pipeline.h"

#include "core/logger/vulkan_ensures.h"

namespace graphics {
vk::PipelineLayout createPipelineLayout(
	vk::Device device,
	std::span<const vk::DescriptorSetLayout> setLayouts,
	std::span<const vk::PushConstantRange> pushConstantRanges
) {
	const vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
		{},
		setLayouts.size(),
		setLayouts.data(),
		pushConstantRanges.size(),
		pushConstantRanges.data()
	);
	const vk::ResultValue<vk::PipelineLayout> pipelineLayoutCreation =
		device.createPipelineLayout(pipelineLayoutInfo);
	VULKAN_ENSURE_SUCCESS(
		pipelineLayoutCreation.result, "Can't create pipeline layout:"
	);
	return pipelineLayoutCreation.value;
}

}  // namespace graphics

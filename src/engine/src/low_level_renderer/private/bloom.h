#pragma once

#include "low_level_renderer/bloom.h"
#include "low_level_renderer/graphics_device_interface.h"

namespace graphics {
BloomData createBloomData(
	const GraphicsDeviceInterface& device,
	const BloomSettings& settings,
	vk::Extent2D swapchainExtent,
	uint32_t swapchainSize
);
void destroy(const BloomData& bloomData, vk::Device device);

vk::RenderPass createBloomRenderpass(
	vk::Device device, vk::Format colorAttachmentFormat
);

BloomPipeline createBloomPipeline();
void destroy(const BloomPipeline& bloomPipeline, vk::Device device);
}  // namespace graphics

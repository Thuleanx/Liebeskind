#pragma once

#include "core/logger/assert.h"
#include "low_level_renderer/bloom.h"
#include "low_level_renderer/graphics_module.h"
#include "low_level_renderer/shaders.h"

namespace graphics {

inline size_t getBloomSampleMip(size_t pass) {
    ASSERT(pass >= 0 && pass < NUM_BLOOM_PASSES, "Invalid pass number: " << pass);
    return pass >= NUM_BLOOM_MIPS ? NUM_BLOOM_PASSES - pass : pass;
}

inline size_t getBloomRenderMip(size_t pass) {
    ASSERT(pass >= 0 && pass < NUM_BLOOM_PASSES, "Invalid pass number: " << pass);
    return pass + 1 >= NUM_BLOOM_MIPS ? NUM_BLOOM_PASSES - (pass + 1) : pass + 1;
}

BloomGraphicsObjects createBloomObjects(
	vk::Device device,
	vk::PhysicalDevice physicalDevice,
	ShaderStorage& shaders,
	vk::Format colorFormat
);
void destroy(BloomGraphicsObjects& objects, vk::Device device);

struct BloomSwapchainObjectCreateInfo {
	vk::Device device;
	vk::PhysicalDevice physicalDevice;
	const Texture& colorBuffer;
	vk::Extent2D swapchainExtent;
	BloomGraphicsObjects& bloomGraphicsObjects;
	vk::Sampler linearSampler;
};
BloomGraphicsObjects::SwapchainObject createBloomSwapchainObject(
	BloomSwapchainObjectCreateInfo createInfo
);
void destroyBloomSwapchainObject(
	BloomGraphicsObjects& graphicsObjects, vk::Device device
);

void recordBloomRenderpass(
	Module& module,
	RenderSubmission& renderSubmission,
	vk::CommandBuffer buffer
);

void updateConfigOnGPU(const BloomGraphicsObjects& obj);

}  // namespace graphics

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

struct BloomSwapchainObjectsCreateInfo {
	vk::Device device;
	vk::PhysicalDevice physicalDevice;
	std::span<const Texture> colorBuffers;
	vk::Extent2D swapchainExtent;
	BloomGraphicsObjects& bloomGraphicsObjects;
	vk::Sampler linearSampler;
};
std::vector<BloomGraphicsObjects::SwapchainObject> createBloomSwapchainObjects(
	BloomSwapchainObjectsCreateInfo createInfo
);
void destroyBloomSwapchainObjects(
	BloomGraphicsObjects& graphicsObjects, vk::Device device
);

void recordBloomRenderpass(
	Module& module,
	RenderSubmission& renderSubmission,
	vk::CommandBuffer buffer,
	uint32_t imageIndex
);

void updateConfigOnGPU(const BloomGraphicsObjects& obj);

}  // namespace graphics

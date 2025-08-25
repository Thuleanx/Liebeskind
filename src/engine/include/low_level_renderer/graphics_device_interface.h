#pragma once

#include <vulkan/vulkan.hpp>
#include <SDL3/SDL.H>
#include <SDL3/SDL_vulkan.h>

#include <optional>

#include "low_level_renderer/config.h"
#include "low_level_renderer/data_buffer.h"
#include "low_level_renderer/material_pipeline.h"
#include "low_level_renderer/queue_family.h"
#include "low_level_renderer/sampler.h"
#include "low_level_renderer/shader_data.h"
#include "low_level_renderer/shaders.h"
#include "low_level_renderer/swapchain_data.h"

constexpr char APP_SHORT_NAME[] = "Game";
constexpr char ENGINE_NAME[] = "Liebeskind";

namespace graphics {
struct GraphicsDeviceInterface {
	struct FrameData {
		vk::DescriptorSet globalDescriptor;
		vk::DescriptorSet postProcessingDescriptor;
		UniformBuffer<GPUSceneData> sceneDataBuffer;
		vk::CommandBuffer drawCommandBuffer;
		vk::Semaphore isImageAvailable;
		vk::Semaphore isRenderingFinished;
		vk::Fence isRenderingInFlight;
	};
	std::array<FrameData, MAX_FRAMES_IN_FLIGHT> frameDatas;

	SDL_Window* window;

	vk::Instance instance;
	vk::DebugUtilsMessengerEXT debugUtilsMessenger;
	vk::SurfaceKHR surface;
	vk::Device device;
	vk::PhysicalDevice physicalDevice;
	QueueFamilyIndices queueFamily;
	vk::Queue graphicsQueue, presentQueue;

	RenderPassData renderPasses;
	MaterialPipeline pipeline;
	struct Shaders {
		UncompiledShader vertex;
		UncompiledShader vertexInstanced;
		UncompiledShader fragment;
	};
	Shaders mainShaders;
	std::optional<SwapchainData> swapchain;
    BloomGraphicsObjects bloom;

	vk::CommandPool commandPool;
	Samplers samplers;

	uint32_t currentFrame = 0;
	DescriptorWriteBuffer writeBuffer;

   public:
	static GraphicsDeviceInterface createGraphicsDevice(ShaderStorage& shaders);
	void destroy();

	void handleEvent(const SDL_Event& sdlEvent);

	void recreateSwapchain();

   private:
	void cleanupSwapchain();
	void handleWindowResize(int width, int height);

   public:
	// Constructors
	[[nodiscard]]
	SwapchainData createSwapchain();
	void destroy(SwapchainData& swapchainData);
};
}  // namespace graphics

#pragma once

#include <vulkan/vulkan.hpp>
#include <SDL3/SDL.H>
#include <SDL3/SDL_vulkan.h>

#include <optional>

#include "low_level_renderer/config.h"
#include "low_level_renderer/material_pipeline.h"
#include "low_level_renderer/queue_family.h"
#include "low_level_renderer/sampler.h"
#include "low_level_renderer/shader_data.h"
#include "low_level_renderer/swapchain_data.h"
#include "low_level_renderer/uniform_buffer.h"
#include "resource_management/resource_manager.h"

constexpr char APP_SHORT_NAME[] = "Game";
constexpr char ENGINE_NAME[] = "Liebeskind";

struct GraphicsDeviceInterface {
    struct FrameData {
        vk::DescriptorSet globalDescriptor;
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

    vk::RenderPass renderPass;
    MaterialPipeline instancedPipeline;
    MaterialPipeline nonInstancedPipeline;
    std::optional<SwapchainData> swapchain;

    vk::CommandPool commandPool;
    Sampler sampler;

    uint32_t currentFrame = 0;
    DescriptorWriteBuffer writeBuffer;

   public:
    static GraphicsDeviceInterface createGraphicsDevice(ResourceManager& resources);
    void destroy();

    void handleEvent(const SDL_Event& sdlEvent);
    void recreateSwapchain();

   private:
    void cleanupSwapchain();
    void handleWindowResize(int width, int height);

   public:
    // Constructors
    [[nodiscard]]
    SwapchainData createSwapchain() const;
    void destroy(SwapchainData& swapchainData) const;
};

#pragma once

#include <vulkan/vulkan.hpp>
#include <SDL3/SDL.H>
#include <SDL3/SDL_vulkan.h>

#include <optional>
#include <vector>

#include "low_level_renderer/descriptor_allocator.h"
#include "low_level_renderer/mesh.h"
#include "low_level_renderer/sampler.h"
#include "low_level_renderer/swapchain_data.h"
#include "low_level_renderer/transformation.h"
#include "low_level_renderer/uniform_buffer.h"

constexpr char APP_SHORT_NAME[] = "Game";
constexpr char ENGINE_NAME[] = "Liebeskind";
constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

class GraphicsDeviceInterface {
    struct FrameData {
        vk::CommandBuffer drawCommandBuffer;
        vk::Semaphore isImageAvailable;
        vk::Semaphore isRenderingFinished;
        vk::Fence isRenderingInFlight;
    };

   public:
    static GraphicsDeviceInterface createGraphicsDevice();
    ~GraphicsDeviceInterface();

    // This makes the object move only (non copyable)
    GraphicsDeviceInterface(GraphicsDeviceInterface&&) = default;
    GraphicsDeviceInterface& operator=(GraphicsDeviceInterface&&) = default;

    GraphicsDeviceInterface() = delete;
    GraphicsDeviceInterface(const GraphicsDeviceInterface&) = delete;
    const GraphicsDeviceInterface& operator=(const GraphicsDeviceInterface&) =
        delete;

    bool drawFrame();
    void handleEvent(const SDL_Event& sdlEvent);

   private:
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> frameDatas;

    SDL_Window* window;

    vk::Instance instance;
    vk::DebugUtilsMessengerEXT debugUtilsMessenger;
    vk::SurfaceKHR surface;

    vk::Device device;
    vk::PhysicalDevice physicalDevice;
    vk::Queue graphicsQueue, presentQueue;

    vk::RenderPass renderPass;
    vk::DescriptorSetLayout descriptorSetLayout;
    DescriptorAllocator descriptorAllocator;
    std::vector<vk::DescriptorSet> descriptorSets;
    vk::PipelineLayout pipelineLayout;
    vk::Pipeline pipeline;

    std::optional<SwapchainData> swapchain;

    std::vector<vk::ShaderModule> shaderModules;

    vk::CommandPool commandPool;

    std::vector<UniformBuffer<ModelViewProjection>> uniformBuffers;
    Mesh mesh;
    Sampler sampler;
    uint32_t currentFrame = 0;

   private:
    GraphicsDeviceInterface(
        std::array<FrameData, MAX_FRAMES_IN_FLIGHT> frameDatas,
        SDL_Window* window,
        vk::Instance instance,
        vk::DebugUtilsMessengerEXT debugUtilsMessenger,
        vk::SurfaceKHR surface,
        vk::Device device,
        vk::PhysicalDevice physicalDevice,
        vk::Queue graphicsQueue,
        vk::Queue presentQueue,
        vk::RenderPass renderPass,
        vk::DescriptorSetLayout descriptorSetLayout,
        DescriptorAllocator descriptorAllocator,
        std::vector<vk::DescriptorSet> descriptorSets,
        vk::PipelineLayout pipelineLayout,
        vk::Pipeline pipeline,
        std::vector<vk::ShaderModule> shaderModules,
        vk::CommandPool commandPool,
        std::vector<UniformBuffer<ModelViewProjection>> uniformBuffers,
        Mesh mesh,
        Sampler sampler
    );

    void recordCommandBuffer(vk::CommandBuffer buffer, uint32_t imageIndex);
    void recreateSwapchain();
    void cleanupSwapchain();
    void handleWindowResize(int width, int height);
    ModelViewProjection getCurrentFrameMVP() const;

   public:
    // Constructors
    [[nodiscard]]
    SwapchainData createSwapchain() const;
    void destroy(SwapchainData& swapchainData) const;
};

#pragma once

#include <vulkan/vulkan.hpp>
#include <SDL3/SDL.H>
#include <SDL3/SDL_vulkan.h>

#include <optional>
#include <unordered_map>
#include <vector>

#include "resource_management/mesh_manager.h"
#include "resource_management/shader_manager.h"
#include "resource_management/texture_manager.h"
#include "resource_management/material_manager.h"

#include "low_level_renderer/config.h"
#include "low_level_renderer/material_pipeline.h"
#include "low_level_renderer/sampler.h"
#include "low_level_renderer/shader_data.h"
#include "low_level_renderer/swapchain_data.h"
#include "low_level_renderer/uniform_buffer.h"

constexpr char APP_SHORT_NAME[] = "Game";
constexpr char ENGINE_NAME[] = "Liebeskind";

struct RenderObject {
    MeshID mesh;
    glm::mat4 transform;
};

class GraphicsDeviceInterface {
    struct FrameData {
        vk::DescriptorSet globalDescriptor;
        UniformBuffer<GPUSceneData> sceneDataBuffer;
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

    // Submits render object for the next draw call
    void submitDrawRenderObject(
        RenderObject renderObject, MaterialInstanceID materialInstance
    );
    bool drawFrame(const GPUSceneData& gpuSceneData);
    void handleEvent(const SDL_Event& sdlEvent);

    // Resource management
    [[nodiscard]]
    TextureID loadTexture(const char* filePath);

    [[nodiscard]]
    MeshID loadMesh(const char* filePath);

    [[nodiscard]]
    MaterialInstanceID loadMaterial(
        TextureID albedo, MaterialProperties properties, MaterialPass pass
    );

    float getAspectRatio() const;

   private:
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> frameDatas;

    SDL_Window* window;

    vk::Instance instance;
    vk::DebugUtilsMessengerEXT debugUtilsMessenger;
    vk::SurfaceKHR surface;

    vk::Device device;
    vk::PhysicalDevice physicalDevice;
    vk::Queue graphicsQueue, presentQueue;

    // Resource managers
    MeshManager meshManager;
    TextureManager textureManager;
    ShaderManager shaderManager;
    MaterialManager materialManager;

    vk::RenderPass renderPass;
    MaterialPipeline pipeline;
    std::optional<SwapchainData> swapchain;

    vk::CommandPool commandPool;
    Sampler sampler;

    std::unordered_map<
        MaterialInstanceID,
        std::vector<RenderObject>,
        MaterialInstanceIDHashFunction>
        renderObjects;
    uint32_t currentFrame = 0;
    DescriptorWriteBuffer writeBuffer;

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
        MeshManager meshManager,
        TextureManager textureManager,
        ShaderManager shaderManager,
        MaterialManager materialManager,
        vk::RenderPass renderPass,
        MaterialPipeline pipeline,
        vk::CommandPool commandPool,
        Sampler sampler
    );

    void recordCommandBuffer(vk::CommandBuffer buffer, uint32_t imageIndex);
    void recreateSwapchain();
    void cleanupSwapchain();
    void handleWindowResize(int width, int height);

   public:
    // Constructors
    [[nodiscard]]
    SwapchainData createSwapchain() const;
    void destroy(SwapchainData& swapchainData) const;

    friend class MeshManager;
};

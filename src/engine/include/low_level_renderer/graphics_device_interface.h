#pragma once

#include "low_level_renderer/vertex_buffer.h"

#include <vulkan/vulkan.hpp>
#include <SDL3/SDL.H>
#include <SDL3/SDL_vulkan.h>

#include <vector>

constexpr char        APP_SHORT_NAME[] = "Game";
constexpr char        ENGINE_NAME[]    = "Liebeskind";
const extern uint32_t MAX_FRAMES_IN_FLIGHT;

class GraphicsDeviceInterface {
   public:
    static GraphicsDeviceInterface createGraphicsDevice();
    ~GraphicsDeviceInterface();

    GraphicsDeviceInterface(const GraphicsDeviceInterface&) = delete;
    const GraphicsDeviceInterface& operator=(const GraphicsDeviceInterface&) =
        delete;

    bool drawFrame();
    void handleEvent(const SDL_Event& sdlEvent);

   private:
    SDL_Window* window;

    vk::Instance               instance;
    vk::DebugUtilsMessengerEXT debugUtilsMessenger;
    vk::SurfaceKHR             surface;

    vk::Device         device;
    vk::PhysicalDevice physicalDevice;
    vk::Queue          graphicsQueue, presentQueue;

    vk::SwapchainKHR           swapchain;
    std::vector<vk::Image>     swapchainImages;
    std::vector<vk::ImageView> swapchainImageViews;
    vk::Format                 swapchainImageFormat;
    vk::Extent2D               swapchainExtent;

    vk::RenderPass     renderPass;
    vk::PipelineLayout pipelineLayout;
    vk::Pipeline       pipeline;

    std::vector<vk::ShaderModule> shaderModules;
    std::vector<vk::Framebuffer>  swapchainFramebuffers;

    vk::CommandPool                commandPool;
    std::vector<vk::CommandBuffer> commandBuffers;

    std::vector<vk::Semaphore> isImageAvailable;
    std::vector<vk::Semaphore> isRenderingFinished;
    std::vector<vk::Fence>     isRenderingInFlight;

    VertexBuffer vertexBuffer;

    uint32_t currentFrame = 0;

   private:
    GraphicsDeviceInterface(
        SDL_Window*                    window,
        vk::Instance                   instance,
        vk::DebugUtilsMessengerEXT     debugUtilsMessenger,
        vk::SurfaceKHR                 surface,
        vk::Device                     device,
        vk::PhysicalDevice             physicalDevice,
        vk::Queue                      graphicsQueue,
        vk::Queue                      presentQueue,
        vk::SwapchainKHR               swapchain,
        std::vector<vk::Image>         swapchainImages,
        std::vector<vk::ImageView>     swapchainImageViews,
        vk::Format                     swapchainImageFormat,
        vk::Extent2D                   swapchainExtent,
        vk::RenderPass                 renderPass,
        vk::PipelineLayout             pipelineLayout,
        vk::Pipeline                   pipeline,
        std::vector<vk::ShaderModule>  shaderModules,
        std::vector<vk::Framebuffer>   swapchainFramebuffers,
        vk::CommandPool                commandPool,
        std::vector<vk::CommandBuffer> commandBuffers,
        std::vector<vk::Semaphore>     isImageAvailable,
        std::vector<vk::Semaphore>     isRenderingFinished,
        std::vector<vk::Fence>         isRenderingInFlight,
        VertexBuffer                   vertexBuffer
    );

    void recordCommandBuffer(vk::CommandBuffer buffer, uint32_t imageIndex);
    void recreateSwapchain();
    void cleanupSwapchain();
    void handleWindowResize(int width, int height);
};

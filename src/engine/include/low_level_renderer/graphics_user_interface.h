#pragma once

#include <vulkan/vulkan.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "low_level_renderer/graphics_device_interface.h"

struct GraphicsUserInterface {
    vk::DescriptorPool descriptorPool;
    vk::RenderPass renderPass;
    vk::CommandPool commandPool;
    std::vector<vk::CommandBuffer> commandBuffers;
    std::vector<vk::Framebuffer> framebuffers;
   public:
    static GraphicsUserInterface create(GraphicsDeviceInterface& device);
    void handleEvent(const SDL_Event& event, const GraphicsDeviceInterface& device);
    void recreateRenderpassAndFramebuffers(const GraphicsDeviceInterface& device);
    void destroy(GraphicsDeviceInterface& device);
};

namespace Graphics {
vk::DescriptorPool createUIDescriptorPool(const GraphicsDeviceInterface& device);
vk::RenderPass createUIRenderPass(const GraphicsDeviceInterface& device);
vk::CommandPool createUICommandPool(const GraphicsDeviceInterface& device);
std::vector<vk::CommandBuffer> createUICommandBuffers(
    const GraphicsDeviceInterface& device,
    vk::CommandPool commandPool
);
std::vector<vk::Framebuffer> createUIFramebuffers(
    const GraphicsDeviceInterface& device,
    const vk::RenderPass renderPass
);
}  // namespace Graphics

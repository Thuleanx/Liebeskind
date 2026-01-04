#pragma once

#include <vulkan/vulkan.hpp>

#include "low_level_renderer/texture.h"

namespace graphics {

struct SwapchainData {
    vk::SwapchainKHR swapchain;
    vk::Extent2D extent;
    std::vector<vk::Image> colorAttachments;
    std::vector<vk::ImageView> colorAttachmentViews;
    Texture intermediateColor;
    Texture multisampleColor;
    Texture depth;
    vk::Framebuffer mainFramebuffer;
    std::vector<vk::Framebuffer> postProcessingFramebuffers;
    std::vector<vk::Semaphore> submitSemaphores;

   public:
    float getAspectRatio() const;
};

}  // namespace graphics

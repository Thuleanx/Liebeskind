#pragma once

#include <vulkan/vulkan.hpp>

#include "low_level_renderer/texture.h"

namespace graphics {
struct SwapchainData {
    vk::SwapchainKHR swapchain;
    vk::Extent2D extent;
    std::vector<vk::Image> colorAttachments;
    std::vector<vk::ImageView> colorAttachmentViews;
    std::vector<Texture> multisampleColorAttachments;
    std::vector<Texture> depthAttachments;
    std::vector<vk::Framebuffer> framebuffers;
    vk::Format colorAttachmentFormat;
    vk::Format depthAttachmentFormat;
    uint32_t imageCount;

   public:
    float getAspectRatio() const;
};

}  // namespace graphics

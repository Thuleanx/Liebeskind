#pragma once

#include <vulkan/vulkan.hpp>

#include "low_level_renderer/texture.h"

namespace graphics {

struct SwapchainData {
    vk::SwapchainKHR swapchain;
    vk::Extent2D extent;
    std::vector<vk::Image> colorAttachments;
    std::vector<vk::ImageView> colorAttachmentViews;
    std::vector<Texture> intermediateColorAttachments;
    std::vector<Texture> multisampleColorAttachments;
    std::vector<Texture> depthAttachments;
    std::vector<vk::Framebuffer> mainFramebuffers;
    std::vector<vk::Framebuffer> postProcessingFramebuffers;

   public:
    float getAspectRatio() const;
};

}  // namespace graphics

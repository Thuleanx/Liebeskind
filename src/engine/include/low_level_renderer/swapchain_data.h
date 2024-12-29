#pragma once

#include <vulkan/vulkan.hpp>

#include "low_level_renderer/texture.h"

struct SwapchainData {
    Texture depth;
    vk::SwapchainKHR swapchain;
    vk::Extent2D extent;
    std::vector<vk::Image> colorAttachments;
    std::vector<vk::ImageView> colorAttachmentViews;
    std::vector<vk::Framebuffer> framebuffers;
    vk::Format colorAttachmentFormat;
    vk::Format depthAttachmentFormat;

   public:
    static vk::Format getSuitableColorAttachmentFormat(
        const vk::PhysicalDevice& physicalDevice,
        const vk::SurfaceKHR& surface
    );
    static vk::Format getSuitableDepthAttachmentFormat(
    );
};

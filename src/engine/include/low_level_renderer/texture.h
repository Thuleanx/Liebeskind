#pragma once

#include <vulkan/vulkan.hpp>

struct Texture {
    vk::Image image;
    vk::ImageView imageView;
    vk::DeviceMemory memory;
    vk::Format format;

   public:
    [[nodiscard]]
    static Texture load(
        const char* filePath,
        const vk::Device& device,
        const vk::PhysicalDevice& physicalDevice,
        const vk::CommandPool& commandPool,
        const vk::Queue& graphicsQueue
    );

    [[nodiscard]]
    static Texture create(
        const vk::Device& device,
        const vk::PhysicalDevice& physicalDevice,
        vk::Format format,
        uint32_t width,
        uint32_t height,
        vk::ImageTiling tiling,
        vk::ImageUsageFlags usage,
        vk::ImageAspectFlags aspect
    );

    void transitionLayout(
        const vk::Device& device,
        const vk::CommandPool& commandPool,
        const vk::Queue& graphicsQueue,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout
    );

    vk::DescriptorImageInfo getDescriptorImageInfo(vk::Sampler sampler) const;
    void destroyBy(const vk::Device& device);

   private:
    Texture(
        vk::Image image,
        vk::DeviceMemory deviceMemory,
        vk::ImageView imageView,
        vk::Format format
    );
};

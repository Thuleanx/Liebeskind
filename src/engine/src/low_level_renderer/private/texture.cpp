#include "low_level_renderer/texture.h"

#include "buffer.h"
#include "core/logger/assert.h"
#include "image.h"
#include "stb_image.h"

Texture Texture::load(
    const char* filePath,
    const vk::Device& device,
    const vk::PhysicalDevice& physicalDevice,
    const vk::CommandPool& commandPool,
    const vk::Queue& graphicsQueue
) {
    int width, height, channels;
    stbi_uc* pixels =
        stbi_load(filePath, &width, &height, &channels, STBI_rgb_alpha);
    const vk::DeviceSize size(width * height * 4);

    ASSERT(pixels, "Can't load texture");

    const auto [stagingBuffer, stagingBufferMemory] = Buffer::create(
        device,
        physicalDevice,
        size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent
    );

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, size, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(size));
    vkUnmapMemory(device, stagingBufferMemory);

    stbi_image_free(pixels);

    vk::Format imageFormat = vk::Format::eR8G8B8A8Srgb;

    const auto [textureImage, textureMemory] = Image::createImage(
        device,
        physicalDevice,
        width,
        height,
        imageFormat,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    Image::transitionImageLayout(
        device,
        commandPool,
        graphicsQueue,
        textureImage,
        imageFormat,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal
    );

    Image::copyBufferToImage(
        device,
        commandPool,
        graphicsQueue,
        stagingBuffer,
        textureImage,
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    );

    Image::transitionImageLayout(
        device,
        commandPool,
        graphicsQueue,
        textureImage,
        imageFormat,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal
    );

    device.destroyBuffer(stagingBuffer);
    device.freeMemory(stagingBufferMemory);

    const vk::ImageView imageView = Image::createImageView(
        device, textureImage, imageFormat, vk::ImageAspectFlagBits::eColor
    );

    return Texture(textureImage, textureMemory, imageView, imageFormat);
}

Texture Texture::create(
    const vk::Device& device,
    const vk::PhysicalDevice& physicalDevice,
    vk::Format format,
    uint32_t width,
    uint32_t height,
    vk::ImageTiling tiling,
    vk::ImageUsageFlags usage,
    vk::ImageAspectFlags aspect
) {
    const auto [image, memory] = Image::createImage(
        device,
        physicalDevice,
        width,
        height,
        format,
        tiling,
        usage,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );
    const vk::ImageView imageView =
        Image::createImageView(device, image, format, aspect);
    return Texture(image, memory, imageView, format);
}

Texture::Texture(
    vk::Image image,
    vk::DeviceMemory deviceMemory,
    vk::ImageView imageView,
    vk::Format format
) :
    image(image), imageView(imageView), memory(deviceMemory), format(format) {}

void Texture::destroyBy(const vk::Device& device) {
    device.destroyImageView(imageView);
    device.destroyImage(image);
    device.freeMemory(memory);
}

vk::DescriptorImageInfo Texture::getDescriptorImageInfo(const Sampler& sampler
) const {
    return vk::DescriptorImageInfo(
        sampler.sampler, imageView, vk::ImageLayout::eShaderReadOnlyOptimal
    );
}

void Texture::transitionLayout(
    const vk::Device& device,
    const vk::CommandPool& commandPool,
    const vk::Queue& graphicsQueue,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout
) {
    Image::transitionImageLayout(
        device, commandPool, graphicsQueue, image, format, oldLayout, newLayout
    );
}

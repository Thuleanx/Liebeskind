#include "low_level_renderer/texture.h"

#include "buffer.h"
#include "image.h"
#include "logger/assert.h"
#include "stb_image.h"

Texture Texture::load(
    const char* filePath,
    const vk::Device& device,
    const vk::PhysicalDevice& physicalDevice,
    vk::CommandPool& commandPool,
    vk::Queue& graphicsQueue
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

    const auto [textureImage, textureMemory] = Image::createImage(
        device,
        physicalDevice,
        width,
        height,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    Image::transitionImageLayout(
        device,
        commandPool,
        graphicsQueue,
        textureImage,
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
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal
    );

    device.destroyBuffer(stagingBuffer);
    device.freeMemory(stagingBufferMemory);

    return Texture(textureImage, textureMemory);
}

Texture::Texture(vk::Image image, vk::DeviceMemory deviceMemory) :
    image(image), memory(deviceMemory) {}

void Texture::destroyBy(const vk::Device& device) {
    device.destroyImage(image);
    device.freeMemory(memory);
}

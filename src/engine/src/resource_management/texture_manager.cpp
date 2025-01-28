#include "resource_management/texture_manager.h"

TextureID TextureManager::load(
    const char* filePath,
    vk::Device device,
    vk::PhysicalDevice physicalDevice,
    vk::CommandPool commandPool,
    vk::Queue graphicsQueue
) {
    TextureID id{static_cast<uint32_t>(textures.size())};
    textures.push_back(Texture::load(
        filePath, device, physicalDevice, commandPool, graphicsQueue
    ));
    return id;
}

void TextureManager::bind(
    TextureID texture,
    vk::DescriptorSet descriptorSet,
    int binding,
    vk::Sampler sampler,
    DescriptorWriteBuffer& writeBuffer
) const {
    writeBuffer.writeImage(
        descriptorSet,
        binding,
        textures[texture.index].imageView,
        vk::DescriptorType::eCombinedImageSampler,
        sampler,
        vk::ImageLayout::eShaderReadOnlyOptimal
    );
}

void TextureManager::destroyBy(vk::Device device) {
    for (Texture& texture : textures) texture.destroyBy(device);
}

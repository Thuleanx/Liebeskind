#include "low_level_renderer/texture.h"

#include <algorithm>
#include <cmath>

#include "core/logger/assert.h"
#include "private/buffer.h"
#include "private/image.h"
#include "stb_image.h"

namespace graphics {
Texture loadTextureFromFile(
    std::string_view filePath,
	vk::Device device,
	vk::PhysicalDevice physicalDevice,
	vk::CommandPool commandPool,
	vk::Queue graphicsQueue,
    vk::Format imageFormat
) {
    LLOG_INFO << "Try loading texture at: " << filePath;

	int width, height, channels;
	stbi_uc* pixels =
		stbi_load(filePath.data(), &width, &height, &channels, STBI_rgb_alpha);
	const vk::DeviceSize size(width * height * 4);

	ASSERT(pixels, "Can't load texture at " << filePath);

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

	const int mipLevels =
		static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) +
		1;

	const auto [textureImage, textureMemory] = Image::createImage(
		device,
		physicalDevice,
		width,
		height,
		imageFormat,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
		vk::MemoryPropertyFlagBits::eDeviceLocal,
        vk::SampleCountFlagBits::e1, // We don't need antialiasing on these textures, 
                                     // we're going to perform it on the entire framebuffer
		mipLevels
	);

	Image::transitionImageLayout(
		device,
		commandPool,
		graphicsQueue,
		textureImage,
		imageFormat,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eTransferDstOptimal,
		mipLevels
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

	Image::generateMipMaps(
		device,
		commandPool,
		graphicsQueue,
		textureImage,
		width,
		height,
		mipLevels
	);

	device.destroyBuffer(stagingBuffer);
	device.freeMemory(stagingBufferMemory);

	const vk::ImageView imageView = Image::createImageView(
		device, textureImage, imageFormat, vk::ImageAspectFlagBits::eColor
	);

    LLOG_INFO << "Finished loading texture at " << filePath;

	return Texture{textureImage, imageView, textureMemory, imageFormat, 1};
}

Texture createTexture(
	vk::Device device,
	vk::PhysicalDevice physicalDevice,
	vk::Format format,
	uint32_t width,
	uint32_t height,
	vk::ImageTiling tiling,
	vk::ImageUsageFlags usage,
	vk::ImageAspectFlags aspect,
    vk::SampleCountFlagBits samplesCount
) {
	const auto [image, memory] = Image::createImage(
		device,
		physicalDevice,
		width,
		height,
		format,
		tiling,
		usage,
		vk::MemoryPropertyFlagBits::eDeviceLocal,
        samplesCount
	);
	const vk::ImageView imageView =
		Image::createImageView(device, image, format, aspect);
	return Texture{image, imageView, memory, format, 1};
}

TextureID pushTextureFromFile(
	TextureStorage& textureStorage,
    std::string_view filePath,
	vk::Device device,
	vk::PhysicalDevice physicalDevice,
	vk::CommandPool commandPool,
	vk::Queue graphicsQueue,
    vk::Format imageFormat
) {
	TextureID id{.index = static_cast<uint32_t>(textureStorage.data.size())};
	textureStorage.data.push_back(loadTextureFromFile(
		filePath, device, physicalDevice, commandPool, graphicsQueue, imageFormat
	));
	return id;
}

void bindTextureToDescriptor(
	const TextureStorage& textureStorage,
	TextureID texture,
	vk::DescriptorSet descriptorSet,
	int binding,
	vk::Sampler sampler,
	DescriptorWriteBuffer& writeBuffer
) {
	writeBuffer.writeImage(
		descriptorSet,
		binding,
		textureStorage.data[texture.index].imageView,
		vk::DescriptorType::eCombinedImageSampler,
		sampler,
		vk::ImageLayout::eShaderReadOnlyOptimal
	);
}

void bindTextureToDescriptor(
    vk::ImageView imageView,
	vk::DescriptorSet descriptorSet,
	int binding,
	vk::Sampler sampler,
	DescriptorWriteBuffer& writeBuffer,
    vk::ImageLayout imageLayout
) {
    writeBuffer.writeImage(
		descriptorSet,
		binding,
        imageView,
		vk::DescriptorType::eCombinedImageSampler,
		sampler,
        imageLayout
    );
}

void transitionLayout(
	const Texture& texture,
	vk::ImageLayout oldLayout,
	vk::ImageLayout newLayout,
	vk::Device device,
	vk::CommandPool commandPool,
	vk::Queue graphicsQueue
) {
	Image::transitionImageLayout(
		device,
		commandPool,
		graphicsQueue,
		texture.image,
		texture.format,
		oldLayout,
		newLayout,
		texture.mipLevels
	);
}

void destroy(TextureStorage& textures, vk::Device device) {
	destroy(textures.data, device);
}

void destroy(std::span<const Texture> textures, vk::Device device) {
	for (const auto [image, imageView, memory, _, __] : textures) {
		device.destroyImageView(imageView);
		device.destroyImage(image);
		device.freeMemory(memory);
	}
}
}  // namespace graphics

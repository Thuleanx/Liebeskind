#include "low_level_renderer/texture.h"

#include <algorithm>
#include <cmath>

#include "core/logger/assert.h"
#include "private/buffer.h"
#include "private/image.h"
#include "stb_image.h"

namespace graphics {

vk::Format getIdealTextureFormat(int channels, const TextureFormatHint& hint) {
	switch (channels) {
		case STBI_grey:
			switch (hint) {
				case TextureFormatHint::eLinear8: return vk::Format::eR8Unorm;
				case TextureFormatHint::eGamma8:  return vk::Format::eR8Srgb;
			}
		case STBI_grey_alpha:
			switch (hint) {
				case TextureFormatHint::eLinear8: return vk::Format::eR8G8Unorm;
				case TextureFormatHint::eGamma8:  return vk::Format::eR8G8Srgb;
			}
		case STBI_rgb:
			switch (hint) {
				case TextureFormatHint::eLinear8:
					return vk::Format::eR8G8B8Unorm;
				case TextureFormatHint::eGamma8: return vk::Format::eR8G8B8Srgb;
			}
		case STBI_rgb_alpha:
			switch (hint) {
				case TextureFormatHint::eLinear8:
					return vk::Format::eR8G8B8A8Unorm;
				case TextureFormatHint::eGamma8:
					return vk::Format::eR8G8B8A8Srgb;
			}
	}
	__builtin_unreachable();
}

Texture loadTextureFromFile(
	std::string_view filePath,
	vk::Device device,
	vk::PhysicalDevice physicalDevice,
	vk::CommandPool commandPool,
	vk::Queue graphicsQueue,
	TextureFormatHint formatHint
) {
	LLOG_INFO << "Try loading texture at: " << filePath;

	int width, height, channels;
	stbi_uc* pixels =
		stbi_load(filePath.data(), &width, &height, &channels, STBI_default);

	const vk::FormatFeatureFlags requiredImageFormatFeatures =
		vk::FormatFeatureFlagBits::eSampledImage |
		vk::FormatFeatureFlagBits::eTransferSrc |
		vk::FormatFeatureFlagBits::eTransferDst;

	const vk::Format idealFormat = getIdealTextureFormat(channels, formatHint);

	const std::array<vk::Format, 5> engineSupportedFormats = {
		idealFormat,
		vk::Format::eR8G8B8A8Srgb,
		vk::Format::eR8G8B8A8Sint,
		vk::Format::eR8G8B8A8Unorm,
		vk::Format::eR8G8B8A8Uint,
	};

	const std::optional<vk::Format> bestFormat = Image::findSupportedFormat(
		physicalDevice,
		engineSupportedFormats,
		vk::ImageTiling::eOptimal,
		requiredImageFormatFeatures
	);

	ASSERT(
		bestFormat.has_value(),
		"No suitable engine format supported for the image at " << filePath
	);

	const vk::Format imageFormat = bestFormat.value();
	const bool isIdealFormatChosen = idealFormat == imageFormat;

	const int bytesPerPixel = (isIdealFormatChosen ? channels : STBI_rgb_alpha);
	const vk::DeviceSize size(width * height * bytesPerPixel);

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

	if (isIdealFormatChosen) {
		memcpy(data, pixels, static_cast<size_t>(size));
	} else {
		const size_t numPixels = static_cast<size_t>(width * height);
		const size_t numBytes = numPixels * bytesPerPixel;

		std::vector<stbi_uc> pixelsPadded;
		pixelsPadded.reserve(numBytes);

		for (size_t i = 0; i < numPixels; i++) {
			for (int j = 0; j < channels; j++)
				pixelsPadded.push_back(pixels[i * channels + j]);
			for (int j = channels; j < STBI_rgb_alpha; j++)
				pixelsPadded.push_back(std::numeric_limits<unsigned char>::max()
				);
		}

		memcpy(data, pixelsPadded.data(), static_cast<size_t>(size));
	}

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
		vk::SampleCountFlagBits::e1,  // We don't need antialiasing on these
									  // textures, we're going to perform it on
									  // the entire framebuffer
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

    constexpr uint32_t mipLevelBase = 0;
	const vk::ImageView imageView = Image::createImageView(
		device,
		textureImage,
		imageFormat,
		vk::ImageAspectFlagBits::eColor,
        mipLevelBase,
		mipLevels
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
	vk::SampleCountFlagBits samplesCount,
	uint32_t mipLevels
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
		samplesCount,
		mipLevels
	);
    constexpr uint32_t mipLevelBase = 0;
	const vk::ImageView imageView =
		Image::createImageView(device, image, format, aspect, mipLevelBase, mipLevels);
	return Texture{image, imageView, memory, format, mipLevels};
}

TextureID pushTextureFromFile(
	TextureStorage& textureStorage,
	std::string_view filePath,
	vk::Device device,
	vk::PhysicalDevice physicalDevice,
	vk::CommandPool commandPool,
	vk::Queue graphicsQueue,
	TextureFormatHint formatHint
) {
	TextureID id{.index = static_cast<uint32_t>(textureStorage.data.size())};
	textureStorage.data.push_back(loadTextureFromFile(
		filePath, device, physicalDevice, commandPool, graphicsQueue, formatHint
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

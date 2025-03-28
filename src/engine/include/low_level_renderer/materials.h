#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include "low_level_renderer/data_buffer.h"
#include "low_level_renderer/descriptor_allocator.h"
#include "low_level_renderer/texture.h"

namespace graphics {

constexpr int MAX_MATERIAL_INSTANCES = 1000;
constexpr std::array<vk::DescriptorSetLayoutBinding, 3> MATERIAL_BINDINGS = {
	vk::DescriptorSetLayoutBinding{
		0,	// binding
		vk::DescriptorType::eUniformBuffer,
		1,	// descriptor count
		vk::ShaderStageFlagBits::eFragment
	},
	vk::DescriptorSetLayoutBinding{
		1,	// binding
		vk::DescriptorType::eCombinedImageSampler,
		1,	// descriptor count
		vk::ShaderStageFlagBits::eFragment
	},
	vk::DescriptorSetLayoutBinding{
		2,	// binding
		vk::DescriptorType::eCombinedImageSampler,
		1,	// descriptor count
		vk::ShaderStageFlagBits::eFragment
	},
};
constexpr std::array<vk::DescriptorPoolSize, 2> MATERIAL_DESCRIPTOR_POOL_SIZES =
	{
		vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1),
		vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 2),
};

struct MaterialInstanceID {
	uint32_t index;

	inline bool operator==(const MaterialInstanceID& other) const {
		return this->index == other.index;
	}
};

struct MaterialInstanceIDHashFunction {
	inline size_t operator()(const MaterialInstanceID& pos) const {
		return pos.index;
	}
};

struct MaterialProperties {
	alignas(16) glm::vec3 specular = glm::vec3(1.0, 1.0, 1.0);
	alignas(16) glm::vec3 diffuse = glm::vec3(1.0, 1.0, 1.0);
	alignas(16) glm::vec3 ambient = glm::vec3(1.0, 1.0, 1.0);
	float shininess = 32;
};

struct MaterialStorage {
	int numOfMaterials = 0;
	std::array<vk::DescriptorSet, MAX_MATERIAL_INSTANCES> descriptors;
	std::array<UniformBuffer<MaterialProperties>, MAX_MATERIAL_INSTANCES>
		uniforms;
};

[[nodiscard]]
MaterialInstanceID create(
	MaterialStorage& materials,
	const TextureStorage& textures,
	TextureID albedo,
	TextureID normalMap,
	const MaterialProperties& materialProperties,
	vk::Device device,
	vk::PhysicalDevice physicalDevice,
	vk::DescriptorSetLayout setLayout,
	vk::Sampler sampler,
	DescriptorAllocator& allocator,
	DescriptorWriteBuffer& writeBuffer
);

void bind(
	const MaterialStorage& materials,
	MaterialInstanceID id,
	vk::CommandBuffer commandBuffer,
	vk::PipelineLayout pipelineLayout
);

void destroy(MaterialStorage& materials, vk::Device device);
}  // namespace graphics

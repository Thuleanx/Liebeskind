#pragma once

#include <optional>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include "core/algo/generation_index_array.h"
#include "low_level_renderer/data_buffer.h"
#include "low_level_renderer/descriptor_allocator.h"
#include "low_level_renderer/pipeline_template.h"
#include "low_level_renderer/sampler.h"
#include "low_level_renderer/texture.h"

namespace graphics {

constexpr int MAX_MATERIAL_INSTANCES = 10000;
constexpr std::array<vk::DescriptorSetLayoutBinding, 5> MATERIAL_BINDINGS = {
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
	vk::DescriptorSetLayoutBinding{
		3,	// binding
		vk::DescriptorType::eCombinedImageSampler,
		1,	// descriptor count
		vk::ShaderStageFlagBits::eFragment
	},
	vk::DescriptorSetLayoutBinding{
		4,	// binding
		vk::DescriptorType::eCombinedImageSampler,
		1,	// descriptor count
		vk::ShaderStageFlagBits::eFragment
	},
};
constexpr std::array<vk::DescriptorPoolSize, 2> MATERIAL_DESCRIPTOR_POOL_SIZES =
	{
		vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1),
		vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 4),
};

enum class DescriptorSetBindingPoint {
	eAlbedo = 1,
	eNormal = 2,
	eDisplacement = 3,
	eEmission = 4
};

using MaterialInstanceID = algo::GenerationIndexPair;

struct MaterialInstanceIDHashFunction {
	inline size_t operator()(const MaterialInstanceID& pos) const {
		return pos.index;
	}
};

struct MaterialProperties {
	alignas(16) glm::vec3 specular = glm::vec3(1.0, 1.0, 1.0);
	alignas(16) glm::vec3 diffuse = glm::vec3(1.0, 1.0, 1.0);
	alignas(16) glm::vec3 ambient = glm::vec3(1.0, 1.0, 1.0);
	alignas(16) glm::vec3 emission = glm::vec3(0.0, 0.0, 0.0);
	alignas(4) float shininess = 32;
};

struct MaterialStorage {
	algo::GenerationIndexArray<MAX_MATERIAL_INSTANCES> indices;
	std::array<vk::DescriptorSet, MAX_MATERIAL_INSTANCES> descriptors;
	std::array<UniformBuffer<MaterialProperties>, MAX_MATERIAL_INSTANCES>
		uniforms;
	std::array<PipelineSpecializationConstants, MAX_MATERIAL_INSTANCES>
		specializationConstant;

   public:
	static MaterialStorage create();
};

struct MaterialCreateInfo {
	std::optional<TextureID> albedo;
	std::optional<TextureID> normal;
	std::optional<TextureID> displacement;
	std::optional<TextureID> emission;
	MaterialProperties materialProperties;
	SamplerType sampler;
};

PipelineSpecializationConstants createSpecializationConstant(
	const MaterialCreateInfo& info
);

[[nodiscard]]
MaterialInstanceID create(
	MaterialStorage& materials,
	const TextureStorage& textures,
	const MaterialCreateInfo& createInfo,
	vk::Device device,
	vk::PhysicalDevice physicalDevice,
	vk::DescriptorSetLayout setLayout,
	vk::Sampler sampler,
	DescriptorAllocator& allocator,
	DescriptorWriteBuffer& writeBuffer
);

void update(
	const MaterialStorage& materials,
	const MaterialProperties& materialProperties,
	MaterialInstanceID instance
);

PipelineSpecializationConstants getSpecializationConstant(
	const MaterialStorage& material, MaterialInstanceID id
);

void bind(
	const MaterialStorage& materials,
	MaterialInstanceID id,
	vk::CommandBuffer commandBuffer,
	vk::PipelineLayout pipelineLayout
);

void destroy(MaterialStorage& materials, vk::Device device);
}  // namespace graphics

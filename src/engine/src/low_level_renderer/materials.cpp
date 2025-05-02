#include "low_level_renderer/materials.h"

#include "core/algo/generation_index_array.h"
#include "core/logger/assert.h"
#include "low_level_renderer/material_pipeline.h"

namespace graphics {

PipelineSpecializationConstants createSpecializationConstant(
	const MaterialCreateInfo& info
) {
	const uint32_t samplerInclusion = static_cast<uint32_t>(
		(info.albedo.has_value() ? SamplerInclusionBits::eAlbedo
								 : SamplerInclusionBits::eNone) |
		(info.normal.has_value() ? SamplerInclusionBits::eNormal
								 : SamplerInclusionBits::eNone) |
		(info.displacement.has_value() ? SamplerInclusionBits::eDisplacement
									   : SamplerInclusionBits::eNone) |
		(info.emission.has_value() ? SamplerInclusionBits::eEmission
								   : SamplerInclusionBits::eNone)
	);
	return {.samplerInclusion = samplerInclusion};
}

MaterialStorage MaterialStorage::create() {
	return {
		.indices = algo::GenerationIndexArray<MAX_MATERIAL_INSTANCES>::create(),
		.descriptors = {},
		.uniforms = {},
		.specializationConstant = {}
	};
}

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
) {
	const MaterialInstanceID id = algo::reserveIndex(materials.indices);

	const UniformBuffer<MaterialProperties> uniformBuffer =
		UniformBuffer<MaterialProperties>::create(device, physicalDevice);
	uniformBuffer.update(createInfo.materialProperties);

    LLOG_INFO << "Allocating descriptor set ";
	const std::vector<vk::DescriptorSet> descriptorSets =
		allocator.allocate(device, setLayout, 1);
	ASSERT(
		descriptorSets.size() == 1,
		"Tried to allocate 1 descriptor set, but received "
			<< descriptorSets.size()
	);
    LLOG_INFO << "Binding descriptor set";
	const vk::DescriptorSet descriptorSet = descriptorSets[0];
	// binding 0 is for material specific properties
	uniformBuffer.bind(writeBuffer, descriptorSet, 0);

    LLOG_INFO << "Binding textures ";
	if (createInfo.albedo.has_value()) {
		bindTextureToDescriptor(
			textures,
			createInfo.albedo.value(),
			descriptorSet,
			static_cast<int>(DescriptorSetBindingPoint::eAlbedo),
			sampler,
			writeBuffer
		);
	}
	if (createInfo.normal.has_value()) {
		bindTextureToDescriptor(
			textures,
			createInfo.normal.value(),
			descriptorSet,
			static_cast<int>(DescriptorSetBindingPoint::eNormal),
			sampler,
			writeBuffer
		);
	}
	if (createInfo.displacement.has_value()) {
		bindTextureToDescriptor(
			textures,
			createInfo.displacement.value(),
			descriptorSet,
			static_cast<int>(DescriptorSetBindingPoint::eDisplacement),
			sampler,
			writeBuffer
		);
	}
	if (createInfo.emission.has_value()) {
		bindTextureToDescriptor(
			textures,
			createInfo.emission.value(),
			descriptorSet,
			static_cast<int>(DescriptorSetBindingPoint::eEmission),
			sampler,
			writeBuffer
		);
	}
	materials.descriptors[id.index] = descriptorSet;
	materials.uniforms[id.index] = uniformBuffer;
	const PipelineSpecializationConstants variant =
		createSpecializationConstant(createInfo);
	materials.specializationConstant[id.index] = variant;
	return id;
}

PipelineSpecializationConstants getSpecializationConstant(
	const MaterialStorage& material, MaterialInstanceID id
) {
	ASSERT(
		algo::isIndexValid(material.indices, id),
		"Invalid material index " << id.index << " " << id.generation
	);
	return material.specializationConstant[id.index];
}

void update(
	const MaterialStorage& materials,
	const MaterialProperties& materialProperties,
	MaterialInstanceID instance
) {
	ASSERT(
		algo::isIndexValid(materials.indices, instance),
		"Invalid material index " << instance.index << " "
								  << instance.generation
	);
	materials.uniforms[instance.index].update(materialProperties);
}

void bind(
	const MaterialStorage& materials,
	MaterialInstanceID id,
	vk::CommandBuffer commandBuffer,
	vk::PipelineLayout pipelineLayout
) {
	commandBuffer.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics,
		pipelineLayout,
		static_cast<int>(MainPipelineDescriptorSetBindingPoint::eMaterial),
		1,
		&materials.descriptors[id.index],
		0,
		nullptr
	);
}

void destroy(MaterialStorage& materials, vk::Device device) {
	for (size_t index : algo::getLiveIndices(materials.indices))
		materials.uniforms[index].destroyBy(device);
}
}  // namespace graphics

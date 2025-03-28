#include "low_level_renderer/materials.h"

#include "low_level_renderer/material_pipeline.h"
#include "core/logger/assert.h"

namespace graphics {
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
) {
	MaterialInstanceID id{
		.index = static_cast<uint32_t>(materials.numOfMaterials++)
	};
	UniformBuffer<MaterialProperties> uniformBuffer =
		UniformBuffer<MaterialProperties>::create(device, physicalDevice);
	uniformBuffer.update(materialProperties);
	std::vector<vk::DescriptorSet> descriptorSets =
		allocator.allocate(device, setLayout, 1);
	ASSERT(
		descriptorSets.size() == 1,
		"Tried to allocate 1 descriptor set, but received "
			<< descriptorSets.size()
	);
	vk::DescriptorSet descriptorSet = descriptorSets[0];
	// binding 0 is for material specific properties
	uniformBuffer.bind(writeBuffer, descriptorSet, 0);
	// binding 1 is for albedo texture
	bindTextureToDescriptor(
		textures, albedo, descriptorSet, 1, sampler, writeBuffer
	);
	// binding 2 is for normal texture
	bindTextureToDescriptor(
		textures, normalMap, descriptorSet, 2, sampler, writeBuffer
	);
	materials.descriptors[id.index] = descriptorSet;
    materials.uniforms[id.index] = uniformBuffer;
	return id;
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
		static_cast<int>(PipelineDescriptorSetBindingPoint::eMaterial),
		1,
		&materials.descriptors[id.index],
		0,
		nullptr
	);
}

void destroy(MaterialStorage& materials, vk::Device device) {
    for (int i = 0; i < materials.numOfMaterials; i++)
        materials.uniforms[i].destroyBy(device);
}
}  // namespace graphics

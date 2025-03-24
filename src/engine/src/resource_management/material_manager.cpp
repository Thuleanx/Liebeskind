#include "resource_management/material_manager.h"

#include "core/logger/assert.h"
#include "low_level_renderer/material_pipeline.h"

size_t MaterialInstanceIDHashFunction ::operator()(
    const MaterialInstanceID& materialInstanceID
) const {
    return materialInstanceID.index |
           (static_cast<size_t>(materialInstanceID.pass) << 32);
}

bool MaterialInstanceID::operator==(const MaterialInstanceID& other) const {
    return index == other.index && pass == other.pass;
}

MaterialInstanceID MaterialManager::load(
    vk::Device device,
    vk::PhysicalDevice physicalDevice,
    vk::DescriptorSetLayout setLayout,
    graphics::DescriptorAllocator& descriptorAllocator,
    vk::Sampler sampler,
    graphics::DescriptorWriteBuffer& writeBuffer,
    graphics::TextureStorage& textureManager,
    graphics::TextureID albedo,
    MaterialProperties materialProperties,
    MaterialPass materialPass
) {
    MaterialInstanceID id{
        static_cast<uint32_t>(
            materialInstances[static_cast<size_t>(materialPass)].size()
        ),
        materialPass
    };
    graphics::UniformBuffer<MaterialProperties> uniformBuffer =
        graphics::UniformBuffer<MaterialProperties>::create(
            device, physicalDevice
        );
    uniformBuffer.update(materialProperties);
    std::vector<vk::DescriptorSet> descriptorSets =
        descriptorAllocator.allocate(device, setLayout, 1);
    ASSERT(
        descriptorSets.size() == 1,
        "Tried to allocate 1 descriptor set, but received "
            << descriptorSets.size()
    );
    vk::DescriptorSet descriptorSet = descriptorSets[0];
    // binding 0 is for material specific properties
    uniformBuffer.bind(writeBuffer, descriptorSet, 0);
    // binding 1 is for albedo texture
    graphics::bindTextureToDescriptor(
        textureManager, albedo, descriptorSet, 1, sampler, writeBuffer
    );
    materialInstances[static_cast<size_t>(materialPass)].push_back(
        {descriptorSet}
    );
    uniforms[static_cast<size_t>(materialPass)].push_back(uniformBuffer);
    return id;
}

void MaterialManager::bind(
    vk::CommandBuffer commandBuffer,
    vk::PipelineLayout pipelineLayout,
    MaterialInstanceID materialID
) const {
    commandBuffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        pipelineLayout,
        static_cast<int>(graphics::PipelineDescriptorSetBindingPoint::eMaterial
        ),
        1,
        &materialInstances[static_cast<size_t>(materialID.pass)]
                          [materialID.index]
                              .descriptor,
        0,
        nullptr
    );
}

void MaterialManager::destroyBy(vk::Device device) {
    for (uint32_t pass = 0; pass <= static_cast<size_t>(MaterialPass::MAX);
         pass++) {
        for (graphics::UniformBuffer<MaterialProperties>& uniformBuffers :
             uniforms[pass])
            uniformBuffers.destroyBy(device);
    }
}

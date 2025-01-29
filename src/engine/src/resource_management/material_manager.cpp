#include "resource_management/material_manager.h"

#include "core/logger/assert.h"

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
    DescriptorAllocator& descriptorAllocator,
    vk::Sampler sampler,
    DescriptorWriteBuffer& writeBuffer,
    TextureManager& textureManager,
    TextureID albedo,
    MaterialProperties materialProperties,
    MaterialPass materialPass
) {
    MaterialInstanceID id{
        static_cast<uint32_t>(materialInstances[materialPass].size()),
        materialPass
    };
    UniformBuffer<MaterialProperties> uniformBuffer =
        UniformBuffer<MaterialProperties>::create(device, physicalDevice);
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
    writeBuffer.writeBuffer(
        descriptorSet,
        0,
        uniformBuffer.buffer,
        vk::DescriptorType::eUniformBuffer,
        0,
        sizeof(MaterialProperties)
    );
    // binding 1 is for albedo texture
    textureManager.bind(albedo, descriptorSet, 1, sampler, writeBuffer);
    materialInstances[materialPass].push_back({descriptorSet});
    uniforms[materialPass].push_back(uniformBuffer);
    return id;
}

void MaterialManager::bind(
    vk::CommandBuffer commandBuffer,
    vk::PipelineLayout pipelineLayout,
    MaterialInstanceID materialID
) {
    commandBuffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        pipelineLayout,
        1,
        1,
        &materialInstances[materialID.pass][materialID.index].descriptor,
        0,
        nullptr
    );
}

void MaterialManager::destroyBy(vk::Device device) {
    for (uint32_t pass = 0; pass <= MaterialPass::MAX; pass++) {
        for (UniformBuffer<MaterialProperties>& uniformBuffers : uniforms[pass])
            uniformBuffers.destroyBy(device);
    }
}

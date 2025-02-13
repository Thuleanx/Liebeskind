#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "resource_management/texture_manager.h"

#include "low_level_renderer/descriptor_allocator.h"
#include "low_level_renderer/descriptor_write_buffer.h"
#include "low_level_renderer/uniform_buffer.h"

enum MaterialPass { OPAQUE, TRANSPARENT, MAX = TRANSPARENT };

struct MaterialInstanceID {
    uint32_t index;
    MaterialPass pass;

    bool operator==(const MaterialInstanceID& other) const;
};

struct MaterialInstanceIDHashFunction {
    size_t operator()(const MaterialInstanceID& pos) const;
};

struct MaterialInstance {
    vk::DescriptorSet descriptor;
};

struct MaterialProperties {
    alignas(16) glm::vec3 specular = glm::vec3(1.0, 1.0, 1.0);
    alignas(16) glm::vec3 diffuse = glm::vec3(1.0, 1.0, 1.0);
    alignas(16) glm::vec3 ambient = glm::vec3(1.0, 1.0, 1.0);
    float shininess = 32;
};

class MaterialManager {
   public:
    [[nodiscard]]
    MaterialInstanceID load(
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
    );
    void bind(
        vk::CommandBuffer commandBuffer,
        vk::PipelineLayout pipelineLayout,
        MaterialInstanceID materialID
    ) const;
    void destroyBy(vk::Device device);

   private:
    std::array<std::vector<MaterialInstance>, MaterialPass::MAX + 1>
        materialInstances;
    std::array<
        std::vector<UniformBuffer<MaterialProperties>>,
        MaterialPass::MAX + 1>
        uniforms;
};

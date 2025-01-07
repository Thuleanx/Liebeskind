#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "low_level_renderer/descriptor_allocator.h"
#include "low_level_renderer/descriptor_write_buffer.h"
#include "low_level_renderer/texture_manager.h"
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
    glm::vec4 color;
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
    );
    void destroyBy(vk::Device device);

   private:
    std::array<std::vector<MaterialInstance>, MaterialPass::MAX + 1>
        materialInstances;
    std::array<
        std::vector<UniformBuffer<MaterialProperties>>,
        MaterialPass::MAX + 1>
        uniforms;
};

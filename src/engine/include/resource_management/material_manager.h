#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "low_level_renderer/descriptor_allocator.h"
#include "low_level_renderer/descriptor_write_buffer.h"
#include "low_level_renderer/data_buffer.h"
#include "low_level_renderer/texture.h"

enum class MaterialPass { OPAQUE = 0, TRANSPARENT = 1, MAX = TRANSPARENT };

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
        graphics::DescriptorAllocator& descriptorAllocator,
        vk::Sampler sampler,
        graphics::DescriptorWriteBuffer& writeBuffer,
        graphics::TextureStorage& textureManager,
        graphics::TextureID albedo,
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
    std::array<std::vector<MaterialInstance>, static_cast<size_t>(MaterialPass::MAX) + 1>
        materialInstances;
    std::array<
        std::vector<graphics::UniformBuffer<MaterialProperties>>,
        static_cast<size_t>(MaterialPass::MAX) + 1>
        uniforms;
};

#pragma once

#include <glm/glm.hpp>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

#include "low_level_renderer/instance_rendering.h"
#include "low_level_renderer/materials.h"
#include "low_level_renderer/meshes.h"

namespace graphics {
struct RenderObject {
    glm::mat4 transform;
    MaterialInstanceID materialInstance;
    MeshID mesh;
};

struct InstancedRenderObject {
    RenderInstanceID instance;
    MaterialInstanceID material;
    MeshID mesh;
    uint16_t count;
};

class RenderSubmission {
    struct SubmittedRenderObject {
        glm::mat4 transform;
        MeshID mesh;
    };

    std::unordered_map<
        MaterialInstanceID,
        std::vector<SubmittedRenderObject>,
        MaterialInstanceIDHashFunction>
        renderObjects;

    std::vector<InstancedRenderObject> instances;
    std::vector<std::vector<InstanceData>> instanceData;

   public:
    static RenderSubmission create();
    void submit(
        std::span<InstancedRenderObject> instances,
        std::span<std::vector<InstanceData>> data
    );
    void submit(std::span<const RenderObject> renderObjects);

    void recordInstanced(
        vk::CommandBuffer buffer,
        vk::PipelineLayout pipelineLayout,
        const RenderInstanceManager& instanceManager,
        const MaterialStorage& materials,
        const MeshStorage& meshes,
        uint32_t currentFrame
    ) const;

    void recordNonInstanced(
        vk::CommandBuffer buffer,
        vk::PipelineLayout pipelineLayout,
        const MaterialStorage& materials,
        const MeshStorage& meshes,
        uint32_t currentFrame
    ) const;
    void clear();
};
}  // namespace Graphics

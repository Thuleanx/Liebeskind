#pragma once

#include <glm/glm.hpp>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

#include "low_level_renderer/instance_rendering.h"
#include "resource_management/material_manager.h"
#include "resource_management/mesh_manager.h"

struct RenderObject {
    glm::mat4 transform;
    MaterialInstanceID materialInstance;
    MeshID mesh;
};

struct InstancedRenderObject {
    glm::mat4 transform;
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

    std::unordered_map<
        RenderInstanceID,
        MaterialInstanceID,
        RenderInstanceIDHashFunction>
        instanceMaterial;
    std::unordered_map<RenderInstanceID, MeshID, RenderInstanceIDHashFunction>
        instanceMesh;
    std::unordered_map<RenderInstanceID, uint16_t, RenderInstanceIDHashFunction>
        instanceNumber;

   public:
    static RenderSubmission create();
    void setInstanceData(
        RenderInstanceID instance,
        MaterialInstanceID material,
        MeshID mesh
    );
    void setInstanceNumber(RenderInstanceID instance, uint16_t number);

    void submit(const RenderObject& renderObject);
    void submit(std::span<const RenderObject> renderObjects);

    void recordInstanced(
        vk::CommandBuffer buffer,
        vk::PipelineLayout pipelineLayout,
        const RenderInstanceManager& instanceManager,
        const MaterialManager& materialManager,
        const MeshManager& meshManager
    ) const;

    void recordNonInstanced(
        vk::CommandBuffer buffer,
        vk::PipelineLayout pipelineLayout,
        const MaterialManager& materialManager,
        const MeshManager& meshManager
    ) const;
    void clear();

   private:
    RenderSubmission() = default;
};

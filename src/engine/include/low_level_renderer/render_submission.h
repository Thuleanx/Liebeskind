#pragma once

#include <glm/glm.hpp>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

#include "resource_management/material_manager.h"
#include "resource_management/mesh_manager.h"

struct RenderObject {
    glm::mat4 transform;
    MaterialInstanceID materialInstance;
    MeshID mesh;
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

   public:
    static RenderSubmission create();
    void submit(RenderObject renderObject);
    void record(
        vk::CommandBuffer buffer,
        vk::PipelineLayout pipelineLayout,
        const MaterialManager& materialManager,
        const MeshManager& meshManager
    ) const;
    void clear();

   private:
    RenderSubmission() = default;
};

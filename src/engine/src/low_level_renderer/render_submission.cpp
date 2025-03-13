#include "low_level_renderer/render_submission.h"

#include "low_level_renderer/shader_data.h"

RenderSubmission RenderSubmission::create() { return RenderSubmission(); }

void RenderSubmission::setInstanceData(
    RenderInstanceID instance,
    MaterialInstanceID material,
    MeshID mesh
) {
    instanceMaterial[instance] = material;
    instanceMesh[instance] = mesh;
}

void RenderSubmission::setInstanceNumber(
    RenderInstanceID instance,
    uint16_t numberOfInstances
) {
    instanceNumber[instance] = numberOfInstances;
}

void RenderSubmission::submit(const RenderObject& renderObject) {
    submit(std::span(&renderObject, 1));
}

void RenderSubmission::submit(std::span<const RenderObject> renderObjects) {
    for (const RenderObject& renderObject : renderObjects) {
        const MaterialInstanceID materialInstanceID =
            renderObject.materialInstance;
        if (!this->renderObjects.contains(materialInstanceID))
            this->renderObjects[materialInstanceID] =
                std::vector<SubmittedRenderObject>();
        this->renderObjects[materialInstanceID].push_back(SubmittedRenderObject{
            .transform = renderObject.transform,
            .mesh = renderObject.mesh,
        });
    }
}
void RenderSubmission::recordInstanced(
    vk::CommandBuffer buffer,
    vk::PipelineLayout pipelineLayout,
    const RenderInstanceManager& instanceManager,
    const MaterialManager& materialManager,
    const MeshManager& meshManager
) const {
    for (const auto& [id, instanceCount] : instanceNumber) {
        materialManager.bind(buffer, pipelineLayout, instanceMaterial.at(id));
        meshManager.bind(buffer, instanceMesh.at(id));
        meshManager.draw(buffer, instanceMesh.at(id), instanceCount);
    }
}

void RenderSubmission::recordNonInstanced(
    vk::CommandBuffer buffer,
    vk::PipelineLayout pipelineLayout,
    const MaterialManager& materialManager,
    const MeshManager& meshManager
) const {
    for (const auto& [materialID, allRenderObjects] : renderObjects) {
        materialManager.bind(buffer, pipelineLayout, materialID);
        for (const SubmittedRenderObject& renderObject : allRenderObjects) {
            GPUPushConstants pushConstants = {.model = renderObject.transform};
            buffer.pushConstants(
                pipelineLayout,
                vk::ShaderStageFlagBits::eVertex,
                0,
                sizeof(GPUPushConstants),
                &pushConstants
            );
            meshManager.bind(buffer, renderObject.mesh);
            meshManager.draw(buffer, renderObject.mesh);
        }
    }
}

void RenderSubmission::clear() { renderObjects.clear(); }

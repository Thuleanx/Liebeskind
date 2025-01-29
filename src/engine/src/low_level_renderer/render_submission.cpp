#include "low_level_renderer/render_submission.h"
#include "low_level_renderer/shader_data.h"

void RenderSubmission::submit(RenderObject renderObject) {
    const MaterialInstanceID materialInstanceID = renderObject.materialInstance;
    if (!renderObjects.contains(materialInstanceID))
        renderObjects[materialInstanceID] =
            std::vector<SubmittedRenderObject>();
    renderObjects[materialInstanceID].push_back(SubmittedRenderObject{
        .transform = renderObject.transform,
        .mesh = renderObject.mesh,
    });
}

void RenderSubmission::record(
    vk::CommandBuffer buffer,
    vk::PipelineLayout pipelineLayout,
    MaterialManager& materialManager,
    MeshManager& meshManager
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

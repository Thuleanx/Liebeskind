#pragma once

#include "SDL3/SDL_events.h"
#include "low_level_renderer/graphics_device_interface.h"
#include "low_level_renderer/graphics_user_interface.h"
#include "low_level_renderer/instance_rendering.h"
#include "low_level_renderer/materials.h"
#include "low_level_renderer/render_submission.h"
#include "resource_management/resource_manager.h"

namespace graphics {
struct Module {
    ResourceManager resources;
    GraphicsDeviceInterface device;
    GraphicsUserInterface ui;
    RenderInstanceManager instances;
    TextureStorage textures;
    MaterialStorage materials;

   public:
    static Module create();
    void destroy();

    void beginFrame();
    void handleEvent(const SDL_Event& event);
    bool drawFrame(
        const RenderSubmission& renderSubmission, GPUSceneData& sceneData
    );
    void endFrame();

    [[nodiscard]] TextureID loadTexture(const char* filePath, vk::Format imageFormat);
    [[nodiscard]] MeshID loadMesh(const char* filePath);
    [[nodiscard]] MaterialInstanceID loadMaterial(
        TextureID albedo,
        TextureID normal,
        TextureID displacementMap,
        TextureID emissionMap,
        MaterialProperties properties,
        SamplerType samplerType
    );
    [[nodiscard]] RenderInstanceID registerInstance(uint16_t numberOfEntries);

   private:
    void recordCommandBuffer(const RenderSubmission& renderSubmission, vk::CommandBuffer, uint32_t image_index);
};
}  // namespace Graphics

#pragma once

#include "SDL3/SDL_events.h"
#include "low_level_renderer/graphics_device_interface.h"
#include "low_level_renderer/graphics_user_interface.h"
#include "low_level_renderer/instance_rendering.h"
#include "low_level_renderer/render_submission.h"
#include "resource_management/resource_manager.h"

struct GraphicsModule {
    ResourceManager resources;
    GraphicsDeviceInterface device;
    GraphicsUserInterface ui;
    RenderInstanceManager instances;

   public:
    static GraphicsModule create();
    void destroy();

    void beginFrame();
    void handleEvent(const SDL_Event& event);
    bool drawAndEndFrame(
        const RenderSubmission& renderSubmission,
        GPUSceneData& sceneData
    );

    [[nodiscard]] TextureID loadTexture(const char* filePath);
    [[nodiscard]] MeshID loadMesh(const char* filePath);
    [[nodiscard]] MaterialInstanceID loadMaterial(
        TextureID albedo, MaterialProperties properties, MaterialPass pass
    );
    [[nodiscard]] RenderInstanceID registerInstance(uint16_t numberOfEntries);
};

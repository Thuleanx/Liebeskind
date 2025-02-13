#pragma once

#include "low_level_renderer/graphics_device_interface.h"
#include "resource_management/resource_manager.h"

struct GraphicsModule {
    ResourceManager resources;
    GraphicsDeviceInterface device;

   public:
    void init();
    void destroy();

    bool drawFrame(const RenderSubmission& renderSubmission, GPUSceneData& sceneData);

    [[nodiscard]] TextureID loadTexture(const char* filePath);
    [[nodiscard]] MeshID loadMesh(const char* filePath);
    [[nodiscard]] MaterialInstanceID loadMaterial(
            TextureID albedo, MaterialProperties properties, MaterialPass pass
            );

    GraphicsModule() :
        resources(),
        device(GraphicsDeviceInterface::createGraphicsDevice(resources)) {}
};

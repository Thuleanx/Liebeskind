#pragma once

#include "game_specific/cameras/perspective_camera.h"
#include "low_level_renderer/graphics_module.h"

class SceneDrawer {
   public:
    PerspectiveCamera camera;
    RenderSubmission renderSubmission;
    std::vector<RenderObject> renderObjects;
    std::vector<InstancedRenderObject> instancedRenderObjects;
    std::vector<std::vector<InstanceData>> instancedRenderData;

   public:
    struct ObjectID {
        uint32_t index;
    };
    static SceneDrawer create();
    void handleResize(int width, int height);
    void handleResize(float aspectRatio);
    bool drawFrame(GraphicsModule& graphics);

    void addInstancedObjects(
        std::span<InstancedRenderObject> instancedRenderObjects
    );
    void updateInstance(
        std::span<int> indices,
        std::vector<std::span<InstanceData>> data
    );
    void addObjects(std::span<RenderObject> renderObjects);
    void updateObjects(std::vector<std::tuple<int, glm::mat4>>);

   private:
    SceneDrawer(SceneDrawer&& device) = default;
    SceneDrawer& operator=(SceneDrawer&&) = default;

    SceneDrawer(PerspectiveCamera camera);
};

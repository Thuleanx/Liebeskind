#pragma once

#include "low_level_renderer/graphics_module.h"

class SceneDrawer {
   public:
    graphics::RenderSubmission renderSubmission;
    std::vector<graphics::RenderObject> renderObjects;
    std::vector<graphics::InstancedRenderObject> instancedRenderObjects;
    std::vector<std::vector<graphics::InstanceData>> instancedRenderData;

   public:
    struct ObjectID {
        uint32_t index;
    };
    static SceneDrawer create();
    bool drawFrame(graphics::Module& graphics);

    void addInstancedObjects(
        std::span<graphics::InstancedRenderObject> instancedRenderObjects
    );
    void updateInstance(
        std::span<int> indices,
        std::vector<std::span<graphics::InstanceData>> data
    );
    void addObjects(std::span<graphics::RenderObject> renderObjects);
    void updateObjects(std::vector<std::tuple<int, glm::mat4>>);
};

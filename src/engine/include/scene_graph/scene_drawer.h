#pragma once

#include "game_specific/cameras/perspective_camera.h"
#include "low_level_renderer/graphics_device_interface.h"

class SceneDrawer {
   public:
    struct ObjectID {
        uint32_t index;
    };
    static SceneDrawer create();
    void handleResize(int width, int height);
    void handleResize(float aspectRatio);
    bool drawFrame(GraphicsDeviceInterface& device);

    void addObjects(std::span<RenderObject> renderObjects);
    void updateObjects(std::vector<std::tuple<int, glm::mat4>>);

   private:
    SceneDrawer(SceneDrawer&& device) = default;
    SceneDrawer& operator=(SceneDrawer&&) = default;

    SceneDrawer(PerspectiveCamera camera);

    PerspectiveCamera camera;
    RenderSubmission renderSubmission;
    std::vector<RenderObject> renderObjects;
};

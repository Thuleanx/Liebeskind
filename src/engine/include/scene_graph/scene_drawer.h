#pragma once

#include "game_specific/cameras/perspective_camera.h"
#include "low_level_renderer/graphics_device_interface.h"

class SceneDrawer {
   public:
    static SceneDrawer create();
    void handleEvent(const SDL_Event& sdlEvent);
    bool drawFrame();

   private:
    SceneDrawer(SceneDrawer&& device) = default;
    SceneDrawer& operator=(SceneDrawer&&) = default;

    SceneDrawer(PerspectiveCamera camera);

    PerspectiveCamera camera;
    RenderSubmission renderSubmission;
    GraphicsDeviceInterface device;
    std::vector<RenderObject> renderObjects;
};

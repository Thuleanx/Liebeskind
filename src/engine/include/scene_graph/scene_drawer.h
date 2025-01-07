#pragma once

#include "low_level_renderer/graphics_device_interface.h"

class SceneDrawer {
   public:
    static SceneDrawer create();
    void handleEvent(const SDL_Event& sdlEvent);
    bool drawFrame();

   private:
    SceneDrawer(SceneDrawer&& device) = default;
    SceneDrawer& operator=(SceneDrawer&&) = default;

    SceneDrawer();

    GraphicsDeviceInterface device;
    std::vector<RenderObject> renderObjects;
    std::vector<MaterialInstanceID> materials;
};

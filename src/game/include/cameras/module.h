#pragma once

#include <optional>
#include "cameras/debug_camera_controller.h"

namespace game_cameras {

struct Module {
	DebugCameraController debugCameraController;

   public:
    static Module create();
    void destroy();

    void update(float deltaTime);

   private:
    static void onMovementX(float value);
    static void onMovementY(float value);
    static void onMouseDeltaX(float value);
    static void onMouseDeltaY(float value);
    static void onJump(bool value);
    static void onCrouch(bool value);
};


extern std::optional<Module> module;

}  // namespace game_cameras

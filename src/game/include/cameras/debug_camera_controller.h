#pragma once

#include "game_specific/cameras/perspective_camera.h"

namespace game_cameras {
struct DebugCameraController {
	float rotationSpeedDegrees;
	float movementSpeed;
	float currentXInput;
	float currentYInput;
    bool upInput;
    bool downInput;
};

void update(
	DebugCameraController& controller,
	cameras::PerspectiveCamera& camera,
	float deltaTime
);

void onMouseDeltaXDebug(
	DebugCameraController& controller,
	cameras::PerspectiveCamera& camera,
	float value
);

void onMouseDeltaYDebug(
	DebugCameraController& controller,
	cameras::PerspectiveCamera& camera,
	float value
);
}  // namespace game_cameras

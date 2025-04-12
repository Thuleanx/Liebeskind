#pragma once

#include "game_specific/cameras/camera.h"

namespace cameras {

struct DebugCameraController {
	float rotationSpeed;
	float movementSpeed;
};

void onRotateX(
	DebugCameraController& controller, Camera& camera, float value
);

void onRotateY(
	DebugCameraController& controller, Camera& camera, float value
);

void onMovementX(
	DebugCameraController& controller, Camera& camera, float value
);

void onMovementY(
	DebugCameraController& controller, Camera& camera, float value
);

void onMovementZ(
	DebugCameraController& controller, Camera& camera, float value
);

void update(DebugCameraController& controller, Camera& camera, float deltaTime);

}  // namespace cameras

#include "cameras/module.h"

#include "cameras/debug_camera_controller.h"
#include "core/logger/assert.h"
#include "game_specific/cameras/module.h"
#include "input_management.h"

namespace game_cameras {
std::optional<Module> module = std::nullopt;

Module Module::create() {
	ASSERT(
		cameras::module, "cameras module must be loaded before game_cameras"
	);
	ASSERT(input::manager, "input module must be loaded before game_cameras");

	Module module = Module{
		.debugCameraController =
			{.rotationSpeedDegrees = 180.f,
			 .movementSpeed = 3,
			 .currentXInput = 0,
			 .currentYInput = 0,
			 .upInput = false,
			 .downInput = false}
	};

	input::manager->subscribe(input::Ranged::MouseX, onMouseDeltaX);
	input::manager->subscribe(input::Ranged::MouseY, onMouseDeltaY);

	input::manager->subscribe(input::Ranged::MovementX, onMovementX);
	input::manager->subscribe(input::Ranged::MovementY, onMovementY);

	input::manager->subscribe(input::Toggled::Jump, onJump);
	input::manager->subscribe(input::Toggled::Crouch, onCrouch);

	return module;
}
void Module::destroy() {
	ASSERT(
		cameras::module,
		"cameras module must be unloaded after game_cameras module"
	);
	ASSERT(input::manager, "input module must be unloaded after game_cameras");
}

void Module::update(float deltaTime) {
	ASSERT(cameras::module, "cameras module has not been loaded");

	game_cameras::update(
		module->debugCameraController, cameras::module->mainCamera, deltaTime
	);
}

void Module::onJump(bool value) {
	ASSERT(module, "game_cameras module has not been loaded");

	module->debugCameraController.upInput = value;
}

void Module::onCrouch(bool value) {
	ASSERT(module, "game_cameras module has not been loaded");

	module->debugCameraController.downInput = value;
}

void Module::onMouseDeltaX(float value) {
	ASSERT(module, "game_cameras module has not been loaded");
	ASSERT(cameras::module, "cameras module has not been loaded");

	onMouseDeltaXDebug(
		module->debugCameraController, cameras::module->mainCamera, value
	);
}

void Module::onMouseDeltaY(float value) {
	ASSERT(module, "game_cameras module has not been loaded");
	ASSERT(cameras::module, "cameras module has not been loaded");

	onMouseDeltaYDebug(
		module->debugCameraController, cameras::module->mainCamera, value
	);
}

void Module::onMovementX(float value) {
	ASSERT(module, "game_cameras module has not been loaded");
	ASSERT(cameras::module, "cameras module has not been loaded");

	module->debugCameraController.currentXInput = value;
}

void Module::onMovementY(float value) {
	ASSERT(module, "game_cameras module has not been loaded");
	ASSERT(cameras::module, "cameras module has not been loaded");

	module->debugCameraController.currentYInput = value;
}

}  // namespace game_cameras

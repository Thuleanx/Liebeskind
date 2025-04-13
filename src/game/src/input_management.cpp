#include "input_management.h"

namespace input {
std::optional<Manager> manager = std::nullopt;

Manager Manager::create() { return {}; }

void Manager::subscribe(Instant input, std::function<void()> listener) {
	instantInputs.Register(input, listener);
}

void Manager::subscribe(Toggled input, std::function<void(bool)> listener) {
	toggledInputs.Register(input, listener);
}

void Manager::subscribe(Ranged input, std::function<void(float)> listener) {
	rangedInputs.Register(input, listener);
}

void Manager::handleEvent(SDL_Event sdlEvent) {
	switch (sdlEvent.type) {
		case SDL_EVENT_KEY_DOWN: onKeyDown(sdlEvent.key.scancode); break;
		case SDL_EVENT_KEY_UP:	 onKeyUp(sdlEvent.key.scancode); break;
		case SDL_EVENT_MOUSE_MOTION:
			if (sdlEvent.motion.xrel != 0)
				rangedInputs.Trigger(Ranged::MouseX, sdlEvent.motion.xrel);
			if (sdlEvent.motion.yrel != 0)
				rangedInputs.Trigger(Ranged::MouseY, sdlEvent.motion.yrel);
			break;
	}
}

void Manager::onKeyDown(SDL_Scancode key) {
	keyState[key] = true;

	switch (key) {
		case SDL_Scancode::SDL_SCANCODE_D:
		case SDL_Scancode::SDL_SCANCODE_A: onMovementXChange(); break;
		case SDL_Scancode::SDL_SCANCODE_W:
		case SDL_Scancode::SDL_SCANCODE_S: onMovementYChange(); break;
		case SDL_Scancode::SDL_SCANCODE_Q:
		case SDL_Scancode::SDL_SCANCODE_E: onRotateChange(); break;
		case SDL_Scancode::SDL_SCANCODE_SPACE:
			toggledInputs.Trigger(Toggled::Jump, true);
            break;
		case SDL_Scancode::SDL_SCANCODE_LSHIFT:
			toggledInputs.Trigger(Toggled::Crouch, true);
            break;

		default:						   break;
	}
}

void Manager::onKeyUp(SDL_Scancode key) {
	keyState[key] = false;

	switch (key) {
		case SDL_Scancode::SDL_SCANCODE_D:
		case SDL_Scancode::SDL_SCANCODE_A: onMovementXChange(); break;
		case SDL_Scancode::SDL_SCANCODE_W:
		case SDL_Scancode::SDL_SCANCODE_S: onMovementYChange(); break;
		case SDL_Scancode::SDL_SCANCODE_Q:
		case SDL_Scancode::SDL_SCANCODE_E: onRotateChange(); break;
		case SDL_Scancode::SDL_SCANCODE_SPACE:
			toggledInputs.Trigger(Toggled::Jump, false);
            break;
		case SDL_Scancode::SDL_SCANCODE_LSHIFT:
			toggledInputs.Trigger(Toggled::Crouch, false);
            break;
		default: break;
	}
}

void Manager::onMovementXChange() {
	float value = keyState[SDL_Scancode::SDL_SCANCODE_D] -
				  keyState[SDL_Scancode::SDL_SCANCODE_A];
	rangedInputs.Trigger(Ranged::MovementX, value);
}

void Manager::onMovementYChange() {
	float value = keyState[SDL_Scancode::SDL_SCANCODE_W] -
				  keyState[SDL_Scancode::SDL_SCANCODE_S];
	rangedInputs.Trigger(Ranged::MovementY, value);
}

void Manager::onRotateChange() {
	float value = keyState[SDL_Scancode::SDL_SCANCODE_Q] -
				  keyState[SDL_Scancode::SDL_SCANCODE_E];
	rangedInputs.Trigger(Ranged::Rotate, value);
}
}  // namespace input

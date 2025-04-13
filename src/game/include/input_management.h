#pragma once

#include <optional>

#include "SDL3/SDL_events.h"
#include "core/algo/event_system.h"

namespace input {
enum class Instant {};
enum class Toggled {
	Jump,
	Crouch,
};
enum class Ranged {
	MouseX,
	MouseY,
	MovementX,
	MovementY,
	Rotate,
};

class Manager {
   public:
	static Manager create();
	void handleEvent(SDL_Event sdlEvent);
	void subscribe(Instant input, std::function<void()> listener);
	void subscribe(Toggled input, std::function<void(bool)> listener);
	void subscribe(Ranged input, std::function<void(float)> listener);

   private:
	void onKeyDown(SDL_Scancode key);
	void onKeyUp(SDL_Scancode key);

	void onMovementXChange();
	void onMovementYChange();
	void onRotateChange();

   private:
	EventSystem<Instant> instantInputs;
	EventSystem<Toggled, bool> toggledInputs;
	EventSystem<Ranged, float> rangedInputs;

	std::unordered_map<SDL_Scancode, bool> keyState;
};

extern std::optional<Manager> manager;

}  // namespace input

#pragma once

#include <optional>

#include "SDL3/SDL_events.h"
#include "core/algo/event_system.h"

namespace input {
enum class Instant {
    Escape,
};
enum class Toggled {
	Jump,
	Crouch,
    MouseDown,
};
enum class Ranged {
	MouseX,
	MouseY,
	MovementX,
	MovementY,
	Rotate,
};

enum class Mode {
    Uninitialized,
    GUI,
    Game,
};

class Manager {
   public:
	static Manager create();
	void handleEvent(SDL_Event sdlEvent);
	void subscribe(Instant input, std::function<void()> listener);
	void subscribe(Toggled input, std::function<void(bool)> listener);
	void subscribe(Ranged input, std::function<void(float)> listener);

   private:
    void switchMode(Mode newMode);
	void onKeyDown(SDL_Scancode key);
	void onKeyUp(SDL_Scancode key);

	void onMovementXChange();
	void onMovementYChange();
	void onRotateChange();

   private:
    Mode currentMode = Mode::Uninitialized;
	EventSystem<Instant> instantInputs;
	EventSystem<Toggled, bool> toggledInputs;
	EventSystem<Ranged, float> rangedInputs;

	std::unordered_map<SDL_Scancode, bool> keyState;
};

extern std::optional<Manager> manager;

}  // namespace input

#pragma once

#include "SDL3/SDL_events.h"
#include "core/algo/event_system.h"

namespace Input {
enum class Instant {};
enum class Toggled {};
enum class Ranged {
    MovementX,
    MovementY
};
}  // namespace Input

class InputManager {
   public:
    void handleEvent(SDL_Event sdlEvent);
    void subscribe(Input::Instant input, std::function<void()> listener);
    void subscribe(Input::Toggled input, std::function<void(bool)> listener);
    void subscribe(Input::Ranged input, std::function<void(float)> listener);

   private:
    void onKeyDown(SDL_Scancode key);
    void onKeyUp(SDL_Scancode key);

    inline void onMovementXChange();
    inline void onMovementYChange();

   private:
    EventSystem<Input::Instant> instantInputs;
    EventSystem<Input::Toggled, bool> toggledInputs;
    EventSystem<Input::Ranged, float> rangedInputs;

    std::unordered_map<SDL_Scancode, bool> keyState;
};

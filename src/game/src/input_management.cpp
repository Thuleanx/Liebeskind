#include "input_management.h"

void InputManager::subscribe(
    Input::Instant input, std::function<void()> listener
) {
    instantInputs.Register(input, listener);
}

void InputManager::subscribe(
    Input::Toggled input, std::function<void(bool)> listener
) {
    toggledInputs.Register(input, listener);
}

void InputManager::subscribe(
    Input::Ranged input, std::function<void(float)> listener
) {
    rangedInputs.Register(input, listener);
}

void InputManager::handleEvent(SDL_Event sdlEvent) {
    switch (sdlEvent.type) {
        case SDL_EVENT_KEY_DOWN: onKeyDown(sdlEvent.key.scancode); break;
        case SDL_EVENT_KEY_UP:   onKeyUp(sdlEvent.key.scancode); break;
    }
}

void InputManager::onKeyDown(SDL_Scancode key) {
    keyState[key] = true;

    switch (key) {
        case SDL_Scancode::SDL_SCANCODE_D:
        case SDL_Scancode::SDL_SCANCODE_A: onMovementXChange(); break;
        case SDL_Scancode::SDL_SCANCODE_W:
        case SDL_Scancode::SDL_SCANCODE_S: onMovementYChange(); break;
        default:                           break;
    }
}

void InputManager::onKeyUp(SDL_Scancode key) {
    keyState[key] = false;

    switch (key) {
        case SDL_Scancode::SDL_SCANCODE_D:
        case SDL_Scancode::SDL_SCANCODE_A: onMovementXChange(); break;
        case SDL_Scancode::SDL_SCANCODE_W:
        case SDL_Scancode::SDL_SCANCODE_S: onMovementYChange(); break;
        default: break;
    }
}

void InputManager::onMovementXChange() {
    float value = keyState[SDL_Scancode::SDL_SCANCODE_D] -
                  keyState[SDL_Scancode::SDL_SCANCODE_A];
    rangedInputs.Trigger(Input::Ranged::MovementX, value);
}

void InputManager::onMovementYChange() {
    float value = keyState[SDL_Scancode::SDL_SCANCODE_W] -
                  keyState[SDL_Scancode::SDL_SCANCODE_S];
    rangedInputs.Trigger(Input::Ranged::MovementY, value);
}

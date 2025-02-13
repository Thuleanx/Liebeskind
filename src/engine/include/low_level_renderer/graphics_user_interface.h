#pragma once

#include <vulkan/vulkan.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

struct GraphicsUserInterface {

public:
    void initGUI(SDL_Window* window);
    void signalNewFrame();
public:
    int n;
};

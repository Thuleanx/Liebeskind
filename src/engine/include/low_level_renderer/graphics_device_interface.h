#pragma once

#include <tuple>

#include <vulkan/vulkan.hpp>
#include <SDL3/SDL.H>
#include <SDL3/SDL_vulkan.h>

constexpr char APP_SHORT_NAME[] = "Game";
constexpr char ENGINE_NAME[] = "Liebeskind";

class GraphicsDeviceInterface {
public:
    explicit GraphicsDeviceInterface();
    ~GraphicsDeviceInterface();

    GraphicsDeviceInterface(const GraphicsDeviceInterface &) = delete;
    const GraphicsDeviceInterface &operator=(const GraphicsDeviceInterface &) = delete;

public:
    bool isConstructionSuccessful;

private: 
    SDL_Window* window;

    vk::Instance instance;
    vk::DebugUtilsMessengerEXT debugUtilsMessenger;
    vk::SurfaceKHR surface;

    vk::Device device;
    vk::PhysicalDevice physicalDevice;
    vk::Queue graphicsQueue, presentQueue;

    vk::SwapchainKHR swapchain;
};
#pragma once

#include <optional>
#include <vector>

#include <vulkan/vulkan.hpp>

namespace graphics {
    extern const std::vector<const char*> deviceExtensions;

    std::vector<const char*> getInstanceExtensions();

    vk::SampleCountFlags getUsableSamplesCount(vk::PhysicalDevice device);

    bool isDeviceSuitable(
            const vk::PhysicalDevice& device, const vk::SurfaceKHR &surface);
    bool areRequiredDeviceExtensionsSupported(const vk::PhysicalDevice& device);

    std::optional<vk::PhysicalDevice> getBestPhysicalDevice(
            const vk::Instance& instance, const vk::SurfaceKHR& surface);
}

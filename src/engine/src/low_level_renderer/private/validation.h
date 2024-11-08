#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>

namespace Validation {
    extern const std::vector<const char*> validationLayers;
    extern const bool shouldEnableValidationLayers;

    bool areValidationLayersSupported();

    vk::DebugUtilsMessengerEXT createDebugMessenger(const vk::Instance& instance);
}

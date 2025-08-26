#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

namespace Validation {
constexpr std::array<vk::ValidationFeatureEnableEXT, 2>
	ENABLED_VALIDATION_FEATURES = {
		vk::ValidationFeatureEnableEXT::eBestPractices,
		vk::ValidationFeatureEnableEXT::eGpuAssisted
};

extern const std::vector<const char*> validationLayers;
extern const bool shouldEnableValidationLayers;
extern const bool shouldEnableBestPractices;

bool areValidationLayersSupported();

vk::DebugUtilsMessengerEXT createDebugMessenger(const vk::Instance& instance);

constexpr vk::ValidationFeaturesEXT VALIDATION_FEATURES(
	ENABLED_VALIDATION_FEATURES.size(),
	ENABLED_VALIDATION_FEATURES.data(),
	0,
	nullptr
);
}  // namespace Validation

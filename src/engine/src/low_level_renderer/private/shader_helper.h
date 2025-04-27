#pragma once

#include <vulkan/vulkan.hpp>

namespace graphics {
std::vector<uint32_t> compileFromGLSLToSPIRV(
	std::string_view code, vk::ShaderStageFlagBits shaderStage
);
};

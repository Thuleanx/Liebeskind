#pragma once

#include <vulkan/vulkan.hpp>

#include "low_level_renderer/shaders.h"

namespace graphics {
std::vector<uint32_t> compileFromGLSLToSPIRV(
	const UncompiledShader& uncompiledShader, std::span<const std::string> defines
);
};

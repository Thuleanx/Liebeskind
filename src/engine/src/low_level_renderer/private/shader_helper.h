#pragma once

#include <vulkan/vulkan.hpp>

#include "low_level_renderer/shaders.h"

namespace graphics {
UncompiledShader loadUncompiledShaderFromFile(std::string_view filePath, vk::ShaderStageFlagBits stage);

std::vector<uint32_t> compileFromGLSLToSPIRV(
	const UncompiledShader& uncompiledShader, std::span<const std::string> defines
);
};

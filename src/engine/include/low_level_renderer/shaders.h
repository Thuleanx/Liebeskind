#pragma once

#include <vulkan/vulkan.hpp>

#include "core/algo/generation_index_array.h"

namespace graphics {

constexpr size_t MAX_SHADERS = 1 << 8;

using ShaderID = algo::GenerationIndexPair;

struct UncompiledShader {
    std::string code;
    vk::ShaderStageFlagBits stage;
};

struct ShaderStorage {
	std::array<vk::ShaderModule, MAX_SHADERS> shaders;
	algo::GenerationIndexArray<MAX_SHADERS> indices;

   public:
	static ShaderStorage create();
};

[[nodiscard]]
ShaderID loadFromBytecode(
	ShaderStorage& shaders,
	vk::Device device,
	const uint32_t* code,
	size_t sizeInBytes
);

[[nodiscard]]
ShaderID loadFromBytecode(
	ShaderStorage& shaders, vk::Device device, const std::vector<char>& code
);

vk::ShaderModule getModule(const ShaderStorage& shaders, ShaderID id);
void destroy(const ShaderStorage& shaders, vk::Device device);

}  // namespace graphics

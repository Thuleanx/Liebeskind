#include "low_level_renderer/shaders.h"

#include "core/logger/vulkan_ensures.h"
#include "core/file_system/file.h"

namespace graphics {
ShaderStorage ShaderStorage::create() {
	return {
		.shaders = {},
		.indices = algo::GenerationIndexArray<MAX_SHADERS>::create()
	};
}

ShaderID loadFromBytecode(
	ShaderStorage& shaders,
	vk::Device device,
	const uint32_t* code,
	size_t sizeInBytes
) {
	const algo::GenerationIndexPair index = algo::reserveIndex(shaders.indices);
	const vk::ResultValue<vk::ShaderModule> shaderModuleCreation =
		device.createShaderModule({{}, sizeInBytes, code});
	VULKAN_ENSURE_SUCCESS(
		shaderModuleCreation.result, "Can't create shader module"
	);
	shaders.shaders[index.index] = shaderModuleCreation.value;
	return index;
}

ShaderID loadFromBytecode(
	ShaderStorage& shaders, vk::Device device, const std::vector<char>& code
) {
	return loadFromBytecode(
		shaders,
		device,
		reinterpret_cast<const uint32_t*>(code.data()),
		code.size()
	);
}

ShaderID loadShaderFromFile(
	ShaderStorage& shaders, vk::Device device, std::string_view filePath
) {
	const std::optional<std::vector<char>> bytecode =
		file_system::readFile(filePath);
	ASSERT(
		bytecode.has_value(),
		"Shader needs to be able to be loaded from " << filePath
	);
	return loadFromBytecode(shaders, device, bytecode.value());
}


vk::ShaderModule getModule(const ShaderStorage& shaders, ShaderID id) {
	ASSERT(
		id.index < shaders.shaders.size(),
		"Mesh id is invalid: index (" << id.index << ") out of range [0, "
									  << shaders.shaders.size() << ")"
	);
	ASSERT(
		algo::isIndexValid(shaders.indices, id),
		"Binding a mesh with invalid mesh ID. Either this mesh has been "
		"deleted or the ID is ill-formed"
	);
	return shaders.shaders[id.index];
}

void destroy(const ShaderStorage& shaders, vk::Device device) {
	for (auto index : algo::getLiveIndices(shaders.indices))
		device.destroyShaderModule(shaders.shaders[index]);
}
}  // namespace graphics

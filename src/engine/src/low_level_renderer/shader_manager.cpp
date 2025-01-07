#include "low_level_renderer/shader_manager.h"

#include "file_system/file.h"
#include "logger/assert.h"
#include "private/helpful_defines.h"

ShaderID ShaderManager::load(vk::Device device, const char* filePath) {
    const std::optional<std::vector<char>> shaderCode =
        FileUtilities::readFile(filePath);
    ASSERT(
        shaderCode.has_value(), "Can't read shader code from file: " << filePath
    );
    const std::vector<char> code = shaderCode.value();
    const vk::ResultValue<vk::ShaderModule> shaderModuleCreation =
        device.createShaderModule(
            {{}, code.size(), reinterpret_cast<const uint32_t*>(code.data())}
        );
    VULKAN_ENSURE_SUCCESS(
        shaderModuleCreation.result,
        "Can't create shader module from " << filePath
    );
    const ShaderID id{numberOfShaders};
    shaders[numberOfShaders++] = shaderModuleCreation.value;
    return id;
}

vk::ShaderModule ShaderManager::getModule(ShaderID id) const {
    return shaders[id.index];
}

void ShaderManager::destroyBy(vk::Device device) {
    for (int i = 0; i < numberOfShaders; i++)
        device.destroyShaderModule(shaders[i]);
}

#include "low_level_renderer/graphics_module.h"
#include "core/logger/vulkan_ensures.h"

void GraphicsModule::init() {

}

void GraphicsModule::destroy() {
    VULKAN_ENSURE_SUCCESS_EXPR(
        device.device.waitIdle(), "Can't wait for device idle:"
    );
    resources.meshes.destroyBy(device.device);
    resources.materials.destroyBy(device.device);
    resources.textures.destroyBy(device.device);
    resources.shaders.destroyBy(device.device);
}

bool GraphicsModule::drawFrame(const RenderSubmission& renderSubmission, GPUSceneData& sceneData) {
    return device.drawFrame(renderSubmission, resources, sceneData);
}

TextureID GraphicsModule::loadTexture(
    const char* filePath
) {
    return resources.textures.load(
        filePath,
        device.device,
        device.physicalDevice,
        device.commandPool,
        device.graphicsQueue
    );
}

MeshID GraphicsModule::loadMesh(
    const char* filePath
) {
    return resources.meshes.load(
        device.device,
        device.physicalDevice,
        device.commandPool,
        device.graphicsQueue,
        filePath
    );
}

MaterialInstanceID GraphicsModule::loadMaterial(
    TextureID albedo,
    MaterialProperties properties,
    MaterialPass pass
) {
    return resources.materials.load(
        device.device,
        device.physicalDevice,
        device.pipeline.materialDescriptorSetLayout,
        device.pipeline.materialDescriptorAllocator,
        device.sampler.sampler,
        device.writeBuffer,
        resources.textures,
        albedo,
        properties,
        pass
    );
}


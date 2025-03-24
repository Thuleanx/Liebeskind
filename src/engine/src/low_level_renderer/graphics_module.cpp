#include "low_level_renderer/graphics_module.h"

#include "core/logger/vulkan_ensures.h"
#include "low_level_renderer/_impl_drawing.h"

namespace graphics {
Module Module::create() {
    ResourceManager resources;
    GraphicsDeviceInterface device =
        GraphicsDeviceInterface::createGraphicsDevice(resources);
    GraphicsUserInterface ui = GraphicsUserInterface::create(device);
    return Module{
        .resources = resources, .device = std::move(device), .ui = ui
    };
}

void Module::destroy() {
    VULKAN_ENSURE_SUCCESS_EXPR(
        device.device.waitIdle(), "Can't wait for device idle:"
    );
    resources.meshes.destroyBy(device.device);
    resources.materials.destroyBy(device.device);
    graphics::destroy(textures, device.device);
    resources.shaders.destroyBy(device.device);
    instances.destroyBy(device.device);
    ui.destroy(device);
    device.destroy();
}

void Module::beginFrame() { ::graphics::beginFrame(device, ui); }

void Module::handleEvent(const SDL_Event& event) {
    device.handleEvent(event);
    ui.handleEvent(event, device);
}

bool Module::drawAndEndFrame(
    const RenderSubmission& renderSubmission, GPUSceneData& sceneData
) {
    bool drawSuccess = drawFrame(
        device, ui, renderSubmission, instances, resources, sceneData
    );

    if (!drawSuccess) return false;

    endFrame(device, ui);

    return true;
}

TextureID Module::loadTexture(const char* filePath) {
    return pushTextureFromFile(
        textures,
        filePath,
        device.device,
        device.physicalDevice,
        device.commandPool,
        device.graphicsQueue
    );
}

MeshID Module::loadMesh(const char* filePath) {
    return resources.meshes.load(
        device.device,
        device.physicalDevice,
        device.commandPool,
        device.graphicsQueue,
        filePath
    );
}

MaterialInstanceID Module::loadMaterial(
    TextureID albedo,
    MaterialProperties properties,
    MaterialPass pass,
    SamplerType samplerType
) {
    vk::Sampler sampler = samplerType == SamplerType::eLinear
                              ? device.samplers.linear
                              : device.samplers.point;
    return resources.materials.load(
        device.device,
        device.physicalDevice,
        device.pipeline.materialDescriptor.setLayout,
        device.pipeline.materialDescriptor.allocator,
        sampler,
        device.writeBuffer,
        textures,
        albedo,
        properties,
        pass
    );
}

RenderInstanceID Module::registerInstance(uint16_t numberOfEntries) {
    return instances.create(
        device.device,
        device.physicalDevice,
        device.pipeline.instanceRenderingDescriptor.setLayout,
        device.pipeline.instanceRenderingDescriptor.allocator,
        device.writeBuffer,
        numberOfEntries
    );
}
}  // namespace Graphics

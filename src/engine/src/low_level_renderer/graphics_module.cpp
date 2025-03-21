#include "low_level_renderer/graphics_module.h"

#include "core/logger/vulkan_ensures.h"
#include "low_level_renderer/_impl_drawing.h"

namespace graphics {
GraphicsModule GraphicsModule::create() {
    ResourceManager resources;
    GraphicsDeviceInterface device =
        GraphicsDeviceInterface::createGraphicsDevice(resources);
    GraphicsUserInterface ui = GraphicsUserInterface::create(device);
    return GraphicsModule{
        .resources = resources, .device = std::move(device), .ui = ui
    };
}

void GraphicsModule::destroy() {
    VULKAN_ENSURE_SUCCESS_EXPR(
        device.device.waitIdle(), "Can't wait for device idle:"
    );
    resources.meshes.destroyBy(device.device);
    resources.materials.destroyBy(device.device);
    resources.textures.destroyBy(device.device);
    resources.shaders.destroyBy(device.device);
    instances.destroyBy(device.device);
    ui.destroy(device);
    device.destroy();
}

void GraphicsModule::beginFrame() { ::graphics::beginFrame(device, ui); }

void GraphicsModule::handleEvent(const SDL_Event& event) {
    device.handleEvent(event);
    ui.handleEvent(event, device);
}

bool GraphicsModule::drawAndEndFrame(
    const RenderSubmission& renderSubmission, GPUSceneData& sceneData
) {
    bool drawSuccess = drawFrame(
        device, ui, renderSubmission, instances, resources, sceneData
    );

    if (!drawSuccess) return false;

    endFrame(device, ui);

    return true;
}

TextureID GraphicsModule::loadTexture(const char* filePath) {
    return resources.textures.load(
        filePath,
        device.device,
        device.physicalDevice,
        device.commandPool,
        device.graphicsQueue
    );
}

MeshID GraphicsModule::loadMesh(const char* filePath) {
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
        resources.textures,
        albedo,
        properties,
        pass
    );
}

RenderInstanceID GraphicsModule::registerInstance(uint16_t numberOfEntries) {
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

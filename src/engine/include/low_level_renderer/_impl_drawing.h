#pragma once

#include "low_level_renderer/graphics_device_interface.h"
#include "low_level_renderer/graphics_user_interface.h"
#include "low_level_renderer/render_submission.h"
#include "low_level_renderer/shader_data.h"

namespace graphics {
void beginFrame(
    const GraphicsDeviceInterface& graphicsDevice,
    const GraphicsUserInterface& ui
);

bool drawFrame(
    GraphicsDeviceInterface& graphicsDevice,
    GraphicsUserInterface& ui,
    const RenderSubmission& renderSubmission,
    const RenderInstanceManager& instanceManager,
    const ResourceManager& resources,
    const GPUSceneData& gpuSceneData
);

void recordCommandBuffer(
    const GraphicsDeviceInterface& graphicsDevice,
    const GraphicsUserInterface& ui,
    const RenderSubmission& renderSubmission,
    const RenderInstanceManager& instanceManager,
    const ResourceManager& resources,
    vk::CommandBuffer buffer,
    uint32_t imageIndex
);

void endFrame(
    GraphicsDeviceInterface& graphicsDevice, GraphicsUserInterface& ui
);

}  // namespace graphics

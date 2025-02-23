#include "low_level_renderer/_impl_drawing.h"

#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_vulkan.h"
#include "core/logger/assert.h"
#include "core/logger/vulkan_ensures.h"
#include "imgui.h"

namespace Graphics {
bool drawFrame(
    GraphicsDeviceInterface& graphicsDevice,
    GraphicsUserInterface& ui,
    const RenderSubmission& renderSubmission,
    const ResourceManager& resources,
    const GPUSceneData& gpuSceneData
) {
    ASSERT(
        graphicsDevice.swapchain, "Attempt to draw frame without a swapchain"
    );

    // Render ImGui. By this point all ImGui calls must be processed. If creating UI for debugging
    // this draw frame process, then we would want to move this to another spot
    ImGui::Render();

    graphicsDevice.writeBuffer.batchWrite(graphicsDevice.device);

    GraphicsDeviceInterface::FrameData& currentFrame =
        graphicsDevice.frameDatas[graphicsDevice.currentFrame];
    const vk::Device device = graphicsDevice.device;

    const uint64_t no_time_limit = std::numeric_limits<uint64_t>::max();
    VULKAN_ENSURE_SUCCESS_EXPR(
        graphicsDevice.device.waitForFences(
            1, &currentFrame.isRenderingInFlight, vk::True, no_time_limit
        ),
        "Can't wait for previous frame rendering:"
    );
    const vk::ResultValue<uint32_t> imageIndex = device.acquireNextImageKHR(
        graphicsDevice.swapchain->swapchain,
        no_time_limit,
        currentFrame.isImageAvailable,
        nullptr
    );

    switch (imageIndex.result) {
        case vk::Result::eErrorOutOfDateKHR:
            LLOG_INFO << "Out of date KHR";
            graphicsDevice.recreateSwapchain();
            return true;

        case vk::Result::eSuccess:
        case vk::Result::eSuboptimalKHR: break;

        default:
            LLOG_ERROR << "Error acquiring next image: "
                       << vk::to_string(imageIndex.result);
            return false;
    }

    VULKAN_ENSURE_SUCCESS_EXPR(
        device.resetFences(1, &currentFrame.isRenderingInFlight),
        "Can't reset fence for render:"
    );
    vk::CommandBuffer commandBuffer = currentFrame.drawCommandBuffer;
    commandBuffer.reset();

    currentFrame.sceneDataBuffer.update(gpuSceneData);

    recordCommandBuffer(
        graphicsDevice,
        ui,
        renderSubmission,
        resources,
        commandBuffer,
        imageIndex.value
    );

    const vk::PipelineStageFlags waitStage =
        vk::PipelineStageFlagBits::eColorAttachmentOutput;
    const vk::SubmitInfo submitInfo(
        1,
        &currentFrame.isImageAvailable,
        &waitStage,
        1,
        &currentFrame.drawCommandBuffer,
        1,
        &currentFrame.isRenderingFinished
    );
    VULKAN_ENSURE_SUCCESS_EXPR(
        graphicsDevice.graphicsQueue.submit(
            1, &submitInfo, currentFrame.isRenderingInFlight
        ),
        "Can't submit graphics queue:"
    );
    vk::PresentInfoKHR presentInfo(
        1,
        &currentFrame.isRenderingFinished,
        1,
        &graphicsDevice.swapchain->swapchain,
        &imageIndex.value,
        nullptr
    );

    switch (graphicsDevice.presentQueue.presentKHR(presentInfo)) {
        case vk::Result::eErrorOutOfDateKHR:
        case vk::Result::eSuboptimalKHR:     graphicsDevice.recreateSwapchain();
        case vk::Result::eSuccess:           break;
        default:
            LLOG_ERROR << "Error submitting to present queue: "
                       << vk::to_string(imageIndex.result);
            return false;
    }
    return true;
}

void recordCommandBuffer(
    const GraphicsDeviceInterface& graphicsDevice,
    const GraphicsUserInterface& ui,
    const RenderSubmission& renderSubmission,
    const ResourceManager& resources,
    vk::CommandBuffer buffer,
    uint32_t imageIndex
) {
    ASSERT(graphicsDevice.swapchain, "Attempt to record command buffer");
    vk::CommandBufferBeginInfo beginInfo({}, nullptr);
    VULKAN_ENSURE_SUCCESS_EXPR(
        buffer.begin(beginInfo), "Can't begin recording command buffer:"
    );

    vk::Rect2D screenExtent(
        vk::Offset2D{0, 0}, graphicsDevice.swapchain->extent
    );

    {  // Main Renderpass
        const std::array<vk::ClearValue, 2> clearColors{
            vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f),
            vk::ClearColorValue(1.0f, 0.0f, 0.0f, 0.0f)
        };
        const vk::RenderPassBeginInfo renderPassInfo(
            graphicsDevice.renderPass,
            graphicsDevice.swapchain->framebuffers[imageIndex],
            screenExtent,
            static_cast<uint32_t>(clearColors.size()),
            clearColors.data()
        );
        buffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
        buffer.bindPipeline(
            vk::PipelineBindPoint::eGraphics, graphicsDevice.pipeline.pipeline
        );

        vk::Viewport viewport(
            0.0f,
            0.0f,
            static_cast<float>(graphicsDevice.swapchain->extent.width),
            static_cast<float>(graphicsDevice.swapchain->extent.height),
            0.0f,
            1.0f
        );
        buffer.setViewport(0, 1, &viewport);
        vk::Rect2D scissor(
            vk::Offset2D(0.0f, 0.0f), graphicsDevice.swapchain->extent
        );
        buffer.setScissor(0, 1, &scissor);

        buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            graphicsDevice.pipeline.layout,
            0,
            1,
            &graphicsDevice.frameDatas[graphicsDevice.currentFrame]
                 .globalDescriptor,
            0,
            nullptr
        );

        renderSubmission.record(
            buffer,
            graphicsDevice.pipeline.layout,
            resources.materials,
            resources.meshes
        );

        buffer.endRenderPass();
    }

    {  // UI RenderPass
        const std::array<vk::ClearValue, 1> clearColors = {
            vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f)
        };

        const vk::RenderPassBeginInfo renderPassInfo(
            ui.renderPass,
            ui.framebuffers[imageIndex],
            screenExtent,
            static_cast<uint32_t>(clearColors.size()),
            clearColors.data()
        );

        buffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), buffer);

        buffer.endRenderPass();
    }

    VULKAN_ENSURE_SUCCESS_EXPR(
        buffer.end(), "Can't end recording command buffer:"
    );
}

void beginFrame(
    const GraphicsDeviceInterface& graphicsDevice,
    const GraphicsUserInterface& ui
) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::Begin("General debugging");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::End();
}

void endFrame(
    GraphicsDeviceInterface& graphicsDevice, GraphicsUserInterface& ui
) {
    graphicsDevice.currentFrame =
        (graphicsDevice.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

}  // namespace Graphics

#include "command.h"

#include "helpful_defines.h"

vk::CommandBuffer Command::beginSingleCommand(
    const vk::Device &device, const vk::CommandPool &commandPool
) {
    const vk::CommandBufferAllocateInfo allocInfo(
        commandPool,
        vk::CommandBufferLevel::ePrimary,
        1  // buffer count
    );
    const vk::ResultValue<std::vector<vk::CommandBuffer>>
        commandBufferAllocation = device.allocateCommandBuffers(allocInfo);
    VULKAN_ENSURE_SUCCESS(
        commandBufferAllocation.result, "Can't allocate command buffer"
    );
    const vk::CommandBuffer commandBuffer = commandBufferAllocation.value.at(0);
    const vk::CommandBufferBeginInfo beginInfo(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    );
    VULKAN_ENSURE_SUCCESS_EXPR(
        commandBuffer.begin(beginInfo), "Can't start command buffer recording"
    );
    return commandBuffer;
}

void Command::submitSingleCommand(
    const vk::Device &device,
    const vk::Queue &submitQueue,
    const vk::CommandPool &commandPool,
    vk::CommandBuffer commandBuffer
) {
    VULKAN_ENSURE_SUCCESS_EXPR(
        commandBuffer.end(),
        "Can't end command buffer recording, likely something wrong happened "
        "during recording"
    );

    vk::SubmitInfo submitInfo(
        0, nullptr, nullptr, 1, &commandBuffer, 0, nullptr
    );

    VULKAN_ENSURE_SUCCESS_EXPR(
        submitQueue.submit(1, &submitInfo, nullptr),
        "Error when submitting queue"
    );
    VULKAN_ENSURE_SUCCESS_EXPR(
        submitQueue.waitIdle(), "Error waiting for submit queue to be idle"
    );

    device.freeCommandBuffers(commandPool, 1, &commandBuffer);
}

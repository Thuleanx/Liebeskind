#pragma once

#include <vulkan/vulkan.hpp>

namespace Command {
[[nodiscard]]
vk::CommandBuffer beginSingleCommand(
    const vk::Device& device, const vk::CommandPool& commandPool
);
void submitSingleCommand(
    const vk::Device& device,
    const vk::Queue& submitQueue,
    const vk::CommandPool& commandPool,
    vk::CommandBuffer commandBuffer
);
}  // namespace Command

#include "low_level_renderer/graphics_user_interface.h"

#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"

void GraphicsUserInterface::initGUI(SDL_Window* window) {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    ImGui_ImplSDL3_InitForVulkan(window);

    const std::vector<vk::DescriptorPoolSize> poolSizes = {
        {vk::DescriptorType::eCombinedImageSampler, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE},
        {vk::DescriptorType::eCombinedImageSampler, 1000},
        {vk::DescriptorType::eCombinedImageSampler, 1000},
        {vk::DescriptorType::eCombinedImageSampler, 1000},
        {vk::DescriptorType::eCombinedImageSampler, 1000},
        {vk::DescriptorType::eCombinedImageSampler, 1000},
    };
}

void GraphicsUserInterface::signalNewFrame() {}

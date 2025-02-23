#include "low_level_renderer/graphics_user_interface.h"

#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_vulkan.h"
#include "core/logger/assert.h"
#include "core/logger/vulkan_ensures.h"
#include "imgui.h"

vk::DescriptorPool Graphics::createUIDescriptorPool(
    const GraphicsDeviceInterface& device
) {
    const int SETS_PER_POOL = 1000;

    const std::vector<vk::DescriptorPoolSize> poolSizes = {
        {vk::DescriptorType::eCombinedImageSampler,
         IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE},
        {vk::DescriptorType::eCombinedImageSampler, SETS_PER_POOL},
        {vk::DescriptorType::eCombinedImageSampler, SETS_PER_POOL},
        {vk::DescriptorType::eCombinedImageSampler, SETS_PER_POOL},
        {vk::DescriptorType::eCombinedImageSampler, SETS_PER_POOL},
        {vk::DescriptorType::eCombinedImageSampler, SETS_PER_POOL},
    };

    vk::DescriptorPoolCreateInfo poolInfo(
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        SETS_PER_POOL * poolSizes.size(),
        static_cast<uint32_t>(poolSizes.size()),
        poolSizes.data()
    );

    const vk::ResultValue<vk::DescriptorPool> descriptorPoolCreation =
        device.device.createDescriptorPool(poolInfo);
    VULKAN_ENSURE_SUCCESS(
        descriptorPoolCreation.result, "Can't create descriptor pool:"
    );

    return descriptorPoolCreation.value;
}

vk::RenderPass Graphics::createUIRenderPass(
    const GraphicsDeviceInterface& device
) {
    ASSERT(
        device.swapchain.has_value(),
        "Can't create UI render pass: device swapchain is not set up"
    );
    const vk::AttachmentDescription colorAttachment(
        {},
        device.swapchain->colorAttachmentFormat,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eLoad,  // need UI to be drawn on top of main
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eColorAttachmentOptimal,  // receiving from main
                                                   // renderpass
        vk::ImageLayout::ePresentSrcKHR
    );

    // Color attachment reference
    const vk::AttachmentReference colorAttachmentReference(
        0,  // index of attachment
        vk::ImageLayout::eColorAttachmentOptimal
    );

    // Subpass
    const vk::SubpassDescription subpassDescription(
        {},
        vk::PipelineBindPoint::eGraphics,
        0,
        nullptr,
        1,
        &colorAttachmentReference
    );

    const vk::SubpassDependency dependency(
        vk::SubpassExternal,
        0,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::AccessFlagBits::eColorAttachmentWrite,
        vk::AccessFlagBits::eColorAttachmentWrite
    );
    const vk::RenderPassCreateInfo renderPassInfo(
        {}, 1, &colorAttachment, 1, &subpassDescription, 1, &dependency
    );
    const vk::ResultValue<vk::RenderPass> renderPassCreation =
        device.device.createRenderPass(renderPassInfo);
    VULKAN_ENSURE_SUCCESS(
        renderPassCreation.result, "Can't create renderpass:"
    );
    return renderPassCreation.value;
}

vk::CommandPool Graphics::createUICommandPool(
    const GraphicsDeviceInterface& graphicsDevice
) {
    ASSERT(
        graphicsDevice.queueFamily.graphicsFamily.has_value(),
        "Can't find queue family for device"
    );
    const vk::CommandPoolCreateInfo poolInfo(
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        graphicsDevice.queueFamily.graphicsFamily.value()
    );
    const vk::ResultValue<vk::CommandPool> commandPoolCreation =
        graphicsDevice.device.createCommandPool(poolInfo);
    VULKAN_ENSURE_SUCCESS(
        commandPoolCreation.result, "Can't create command pool:"
    );
    return commandPoolCreation.value;
}

std::vector<vk::CommandBuffer> Graphics::createUICommandBuffers(
    const GraphicsDeviceInterface& device, vk::CommandPool pool
) {
    const vk::CommandBufferAllocateInfo allocateInfo(
        pool, vk::CommandBufferLevel::ePrimary, device.frameDatas.size()
    );
    const vk::ResultValue<std::vector<vk::CommandBuffer>>
        commandBuffersAllocation =
            device.device.allocateCommandBuffers(allocateInfo);
    VULKAN_ENSURE_SUCCESS(
        commandBuffersAllocation.result, "Can't allocate command buffers:"
    );
    ASSERT(
        commandBuffersAllocation.value.size() == MAX_FRAMES_IN_FLIGHT,
        "Allocated " << commandBuffersAllocation.value.size()
                     << " command buffers when requesting "
                     << MAX_FRAMES_IN_FLIGHT
    );
    return commandBuffersAllocation.value;
}

std::vector<vk::Framebuffer> Graphics::createUIFramebuffers(
    const GraphicsDeviceInterface& device, const vk::RenderPass renderPass
) {
    ASSERT(
        device.swapchain.has_value(),
        "Can't create UI Framebuffers without main swapchain"
    );
    const int numberOfBuffers = device.swapchain->colorAttachmentViews.size();
    std::vector<vk::Framebuffer> uiFramebuffers(numberOfBuffers);
    for (int i = 0; i < numberOfBuffers; i++) {
        vk::ImageView frameImage = device.swapchain->colorAttachmentViews[i];
        ASSERT(frameImage, "Swapchain image at location " << i << " is null");
        const std::array<vk::ImageView, 1> attachments = {frameImage};
        const vk::FramebufferCreateInfo framebufferCreateInfo(
            {},
            renderPass,
            static_cast<uint32_t>(attachments.size()),
            attachments.data(),
            device.swapchain->extent.width,
            device.swapchain->extent.height,
            1
        );

        const vk::ResultValue<vk::Framebuffer> framebufferCreation =
            device.device.createFramebuffer(framebufferCreateInfo);
        VULKAN_ENSURE_SUCCESS(
            framebufferCreation.result, "Can't create framebuffer:"
        );
        uiFramebuffers[i] = framebufferCreation.value;
    }

    return uiFramebuffers;
}

GraphicsUserInterface GraphicsUserInterface::create(
    GraphicsDeviceInterface& device
) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    ImGui_ImplSDL3_InitForVulkan(device.window);

    const vk::DescriptorPool descriptorPool =
        Graphics::createUIDescriptorPool(device);
    const vk::RenderPass renderPass = Graphics::createUIRenderPass(device);
    const vk::CommandPool commandPool = Graphics::createUICommandPool(device);
    const std::vector<vk::CommandBuffer> commandBuffers =
        Graphics::createUICommandBuffers(device, commandPool);
    const std::vector<vk::Framebuffer> framebuffers =
        Graphics::createUIFramebuffers(device, renderPass);

    ImGui_ImplVulkan_InitInfo initInfo{
        .Instance = device.instance,
        .PhysicalDevice = device.physicalDevice,
        .Device = device.device,
        .QueueFamily = device.queueFamily.graphicsFamily.value(),
        .Queue = device.graphicsQueue,
        .DescriptorPool = descriptorPool,
        .RenderPass = renderPass,
        .MinImageCount = device.swapchain->imageCount,
        .ImageCount = device.swapchain->imageCount,
    };

    ImGui_ImplVulkan_Init(&initInfo);

    ImGui_ImplVulkan_CreateFontsTexture();

    return GraphicsUserInterface{
        .descriptorPool = descriptorPool,
        .renderPass = renderPass,
        .commandPool = commandPool,
        .commandBuffers = commandBuffers,
        .framebuffers = framebuffers
    };
}

void GraphicsUserInterface::handleEvent(const SDL_Event& event, const GraphicsDeviceInterface& device) {
    ImGui_ImplSDL3_ProcessEvent(&event);
    switch (event.type) {
        case SDL_EVENT_WINDOW_RESIZED:
            recreateRenderpassAndFramebuffers(device);
            break;
    }
}

void GraphicsUserInterface::recreateRenderpassAndFramebuffers(const GraphicsDeviceInterface& device) {
    for (const vk::Framebuffer& framebuffer : framebuffers)
        device.device.destroyFramebuffer(framebuffer);
    device.device.destroyRenderPass(renderPass);

    renderPass = Graphics::createUIRenderPass(device);
    framebuffers = Graphics::createUIFramebuffers(device, renderPass);
}

void GraphicsUserInterface::destroy(GraphicsDeviceInterface& device) {
    ImGui_ImplVulkan_DestroyFontsTexture();
    ImGui_ImplVulkan_Shutdown();

    for (const vk::Framebuffer& framebuffer : framebuffers)
        device.device.destroyFramebuffer(framebuffer);
    device.device.destroyCommandPool(commandPool);
    device.device.destroyRenderPass(renderPass);
    device.device.destroyDescriptorPool(descriptorPool);

    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

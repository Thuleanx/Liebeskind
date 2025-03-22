#include "low_level_renderer/graphics_device_interface.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <set>
#include <vector>

#include "core/logger/assert.h"
#include "core/logger/vulkan_ensures.h"
#include "low_level_renderer/descriptor_write_buffer.h"
#include "low_level_renderer/queue_family.h"
#include "private/graphics_device_helper.h"
#include "private/swapchain.h"
#include "private/validation.h"

namespace graphics {

namespace {

std::tuple<vk::Instance, vk::DebugUtilsMessengerEXT> init_createInstance() {
    const vk::ApplicationInfo appInfo(
        APP_SHORT_NAME,
        VK_MAKE_VERSION(1, 0, 0),
        ENGINE_NAME,
        VK_MAKE_VERSION(1, 0, 0),
        VK_API_VERSION_1_3
    );
    std::vector<const char*> instanceExtensions =
        getInstanceExtensions();
    ASSERT(instanceExtensions.size(), "No supported extensions found");
    ASSERT(
        !Validation::shouldEnableValidationLayers ||
            Validation::areValidationLayersSupported(),
        "Validation layers enabled but not supported"
    );
    const vk::InstanceCreateInfo instanceInfo(
        vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
        &appInfo,
        Validation::shouldEnableValidationLayers
            ? static_cast<uint32_t>(Validation::validationLayers.size())
            : 0,
        Validation::shouldEnableValidationLayers
            ? Validation::validationLayers.data()
            : nullptr,
        instanceExtensions.size(),
        instanceExtensions.data()
    );
    const vk::ResultValue<vk::Instance> instanceCreationResult =
        vk::createInstance(instanceInfo, nullptr);
    VULKAN_ENSURE_SUCCESS(
        instanceCreationResult.result, "Can't create instance:"
    );
    const vk::Instance instance = instanceCreationResult.value;
    vk::DebugUtilsMessengerEXT debugUtilsMessenger;

    if (Validation::shouldEnableValidationLayers)
        debugUtilsMessenger = Validation::createDebugMessenger(instance);

    return std::make_tuple(instance, debugUtilsMessenger);
}

vk::SurfaceKHR init_createSurface(
    SDL_Window* window, const vk::Instance& instance
) {
    VkSurfaceKHR surface;
    bool isSurfaceCreationSuccessful =
        SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface);
    ASSERT(isSurfaceCreationSuccessful, "Cannot create surface");
    return surface;
}

vk::PhysicalDevice init_createPhysicalDevice(
    const vk::Instance& instance, const vk::SurfaceKHR& surface
) {
    std::optional<vk::PhysicalDevice> bestDevice =
        getBestPhysicalDevice(instance, surface);
    ASSERT(bestDevice.has_value(), "No suitable device found");
    return bestDevice.value_or(vk::PhysicalDevice());
}

vk::Device init_createLogicalDevice(
    const vk::PhysicalDevice& physicalDevice,
    const QueueFamilyIndices& queueFamily
) {
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    float queuePriority = 1.0f;
    const std::set<uint32_t> uniqueQueueFamilies = {
        queueFamily.graphicsFamily.value(), queueFamily.presentFamily.value()
    };
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

    for (uint32_t queueFamilyIndex : uniqueQueueFamilies) {
        vk::DeviceQueueCreateInfo deviceQueueCreateInfo(
            {}, queueFamilyIndex, 1, &queuePriority
        );
        queueCreateInfos.push_back(deviceQueueCreateInfo);
    }

    vk::PhysicalDeviceFeatures deviceFeatures;
    deviceFeatures.samplerAnisotropy = vk::True;
    const vk::DeviceCreateInfo deviceCreateInfo(
        {},
        queueCreateInfos.size(),
        queueCreateInfos.data(),
        Validation::shouldEnableValidationLayers
            ? static_cast<uint32_t>(Validation::validationLayers.size())
            : 0,
        Validation::shouldEnableValidationLayers
            ? Validation::validationLayers.data()
            : nullptr,
        static_cast<uint32_t>(deviceExtensions.size()),
        deviceExtensions.data(),
        &deviceFeatures
    );
    vk::Device device;
    vk::Result result =
        physicalDevice.createDevice(&deviceCreateInfo, nullptr, &device);
    VULKAN_ENSURE_SUCCESS(result, "Failed to create device:");
    return device;
}

vk::RenderPass init_createRenderPass(
    const vk::Device& device,
    vk::Format swapchainImageFormat,
    vk::Format depthFormat
) {
    const vk::AttachmentDescription colorAttachment(
        {},
        swapchainImageFormat,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal
        // vk::ImageLayout::ePresentSrcKHR         // we are expecting to pipe
        //  this into UI render pass
    );
    const vk::AttachmentDescription depthAttachment(
        {},
        depthFormat,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eDontCare,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal
    );
    const vk::AttachmentReference colorAttachmentReference(
        0,  // index of attachment
        vk::ImageLayout::eColorAttachmentOptimal
    );
    const vk::AttachmentReference depthAttachmentReference(
        1,  // index of attachment
        vk::ImageLayout::eDepthStencilAttachmentOptimal
    );
    const vk::SubpassDescription subpassDescription(
        {},
        vk::PipelineBindPoint::eGraphics,
        0,
        nullptr,
        1,
        &colorAttachmentReference,
        nullptr,
        &depthAttachmentReference
    );
    const vk::SubpassDependency dependency(
        vk::SubpassExternal,
        0,
        vk::PipelineStageFlagBits::eColorAttachmentOutput |
            vk::PipelineStageFlagBits::eEarlyFragmentTests,
        vk::PipelineStageFlagBits::eColorAttachmentOutput |
            vk::PipelineStageFlagBits::eEarlyFragmentTests,
        {},
        vk::AccessFlagBits::eColorAttachmentWrite |
            vk::AccessFlagBits::eDepthStencilAttachmentWrite
    );
    const std::array<vk::AttachmentDescription, 2> attachments = {
        colorAttachment, depthAttachment
    };
    const vk::RenderPassCreateInfo renderPassInfo(
        {},
        static_cast<uint32_t>(attachments.size()),
        attachments.data(),
        1,
        &subpassDescription,
        1,
        &dependency
    );
    const vk::ResultValue<vk::RenderPass> renderPassCreation =
        device.createRenderPass(renderPassInfo);
    VULKAN_ENSURE_SUCCESS(
        renderPassCreation.result, "Can't create renderpass:"
    );
    return renderPassCreation.value;
}

vk::CommandPool init_createCommandPool(
    const vk::Device& device, const QueueFamilyIndices queueFamilies
) {
    ASSERT(
        queueFamilies.graphicsFamily.has_value(),
        "Calling create command pool on incomplete family indices"
    );
    const vk::CommandPoolCreateInfo poolInfo(
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        queueFamilies.graphicsFamily.value()
    );
    const vk::ResultValue<vk::CommandPool> commandPoolCreation =
        device.createCommandPool(poolInfo);
    VULKAN_ENSURE_SUCCESS(
        commandPoolCreation.result, "Can't create command pool:"
    );
    return commandPoolCreation.value;
}

std::vector<vk::CommandBuffer> init_createCommandBuffers(
    const vk::Device& device,
    const vk::CommandPool& pool,
    const uint32_t num_of_frames
) {
    const vk::CommandBufferAllocateInfo allocateInfo(
        pool, vk::CommandBufferLevel::ePrimary, num_of_frames
    );
    const vk::ResultValue<std::vector<vk::CommandBuffer>>
        commandBuffersAllocation = device.allocateCommandBuffers(allocateInfo);
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

std::tuple<
    std::vector<vk::Semaphore>,
    std::vector<vk::Semaphore>,
    std::vector<vk::Fence>>
init_createSyncObjects(const vk::Device& device, const uint32_t num_of_frames) {
    std::vector<vk::Semaphore> isImageAvailable(num_of_frames);
    std::vector<vk::Semaphore> isRenderingFinished(num_of_frames);
    std::vector<vk::Fence> isRenderingInFlight(num_of_frames);
    vk::SemaphoreCreateInfo semaphoreCreateInfo;
    vk::FenceCreateInfo fenceCreateInfo(vk::FenceCreateFlagBits::eSignaled);

    for (uint32_t i = 0; i < num_of_frames; i++) {
        const vk::ResultValue<vk::Semaphore> isImageAvailableCreation =
            device.createSemaphore(semaphoreCreateInfo);
        const vk::ResultValue<vk::Semaphore> isRenderingFinishedCreation =
            device.createSemaphore(semaphoreCreateInfo);
        const vk::ResultValue<vk::Fence> isRenderingInFlightCreation =
            device.createFence(fenceCreateInfo);
        VULKAN_ENSURE_SUCCESS(
            isImageAvailableCreation.result, "Can't create semaphore:"
        );
        VULKAN_ENSURE_SUCCESS(
            isRenderingFinishedCreation.result, "Can't create semaphore:"
        );
        VULKAN_ENSURE_SUCCESS(
            isRenderingInFlightCreation.result, "Can't create fence:"
        );
        isImageAvailable[i] = isImageAvailableCreation.value;
        isRenderingFinished[i] = isRenderingFinishedCreation.value;
        isRenderingInFlight[i] = isRenderingInFlightCreation.value;
    }

    return std::make_tuple(
        isImageAvailable, isRenderingFinished, isRenderingInFlight
    );
}

template <typename T>
std::vector<UniformBuffer<T>> init_createUniformBuffers(
    const vk::Device& device,
    const vk::PhysicalDevice& physicalDevice,
    const uint32_t numberOfFrames
) {
    std::vector<UniformBuffer<T>> result;
    result.reserve(MAX_FRAMES_IN_FLIGHT);

    for (uint32_t _ = 0; _ < numberOfFrames; _++) {
        result.push_back(UniformBuffer<T>::create(device, physicalDevice));
    }

    return result;
}

}  // namespace
GraphicsDeviceInterface GraphicsDeviceInterface::createGraphicsDevice(
    ResourceManager& resources
) {
    const SDL_InitFlags initFlags = SDL_INIT_VIDEO | SDL_INIT_EVENTS;
    const bool isSDLInitSuccessful = SDL_Init(initFlags);
    ASSERT(
        isSDLInitSuccessful,
        "SDL cannot be initialized with flag " << initFlags
                                               << " error: " << SDL_GetError()
    );
    const bool isVulkanLibraryLoadSuccessful = SDL_Vulkan_LoadLibrary(nullptr);
    ASSERT(isVulkanLibraryLoadSuccessful, "Vulkan library cannot be loaded");
    SDL_Window* window = SDL_CreateWindow(
        "Liebeskind", 1024, 768, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
    );
    const auto [instance, debugUtilsMessenger] = init_createInstance();
    const vk::SurfaceKHR surface = init_createSurface(window, instance);
    const vk::PhysicalDevice physicalDevice =
        init_createPhysicalDevice(instance, surface);
    const QueueFamilyIndices queueFamily =
        QueueFamilyIndices::findQueueFamilies(physicalDevice, surface);
    const vk::Device device =
        init_createLogicalDevice(physicalDevice, queueFamily);
    const vk::Queue graphicsQueue =
        device.getQueue(queueFamily.graphicsFamily.value(), 0);
    const vk::Queue presentQueue =
        device.getQueue(queueFamily.presentFamily.value(), 0);
    const vk::CommandPool commandPool =
        init_createCommandPool(device, queueFamily);

    ShaderID vertexShader =
        resources.shaders.load(device, "shaders/test_triangle.vert.glsl.spv");
    ShaderID instancedVertexShader = resources.shaders.load(
        device, "shaders/test_triangle_instanced.vert.glsl.spv"
    );
    ShaderID fragmentShader =
        resources.shaders.load(device, "shaders/test_triangle.frag.glsl.spv");

    const std::vector<vk::CommandBuffer> commandBuffers =
        init_createCommandBuffers(device, commandPool, MAX_FRAMES_IN_FLIGHT);
    LLOG_INFO << "Created command pool and buffers";

    const Samplers allSamplers =
        Samplers::create(device, physicalDevice);
    const vk::SurfaceFormatKHR swapchainColorFormat =
        Swapchain::getSuitableColorAttachmentFormat(physicalDevice, surface);
    const vk::RenderPass renderPass = init_createRenderPass(
        device,
        swapchainColorFormat.format,
        Swapchain::getSuitableDepthAttachmentFormat(physicalDevice)
    );

    DescriptorWriteBuffer writeBuffer;
    MaterialPipeline pipeline = MaterialPipeline::create(
        device,
        resources.shaders.getModule(vertexShader),
        resources.shaders.getModule(instancedVertexShader),
        resources.shaders.getModule(fragmentShader),
        renderPass
    );

    std::vector<vk::DescriptorSet> globalDescriptors =
        pipeline.globalDescriptor.allocator.allocate(
            device, pipeline.globalDescriptor.setLayout, MAX_FRAMES_IN_FLIGHT
        );
    ASSERT(
        globalDescriptors.size() == MAX_FRAMES_IN_FLIGHT,
        "Requested " << MAX_FRAMES_IN_FLIGHT
                     << " global descriptors but allocator returned "
                     << globalDescriptors.size()
    );
    std::vector<UniformBuffer<GPUSceneData>> uniformBuffers =
        init_createUniformBuffers<GPUSceneData>(
            device, physicalDevice, MAX_FRAMES_IN_FLIGHT
        );
    ASSERT(
        uniformBuffers.size() == MAX_FRAMES_IN_FLIGHT,
        "Tried to create" << MAX_FRAMES_IN_FLIGHT
                          << " uniform buffers but allocator returned "
                          << uniformBuffers.size()
    );
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        uniformBuffers[i].bind(writeBuffer, globalDescriptors[i], 0);
    writeBuffer.batchWrite(device);

    const auto [isImageAvailable, isRenderingFinished, isRenderingInFlight] =
        init_createSyncObjects(device, MAX_FRAMES_IN_FLIGHT);
    LLOG_INFO << "Created semaphore and fences";

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> frameDatas = {};
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        frameDatas[i] = {
            globalDescriptors[i],
            uniformBuffers[i],
            commandBuffers[i],
            isImageAvailable[i],
            isRenderingFinished[i],
            isRenderingInFlight[i]
        };
    }

    GraphicsDeviceInterface deviceInterface{
        .frameDatas = frameDatas,
        .window = window,
        .instance = instance,
        .debugUtilsMessenger = debugUtilsMessenger,
        .surface = surface,
        .device = device,
        .physicalDevice = physicalDevice,
        .queueFamily = queueFamily,
        .graphicsQueue = graphicsQueue,
        .presentQueue = presentQueue,
        .renderPass = renderPass,
        .pipeline = pipeline,
        .swapchain = {},
        .commandPool = commandPool,
        .samplers = allSamplers,
        .currentFrame = 0,
        .writeBuffer = writeBuffer
    };

    deviceInterface.swapchain = deviceInterface.createSwapchain();

    return deviceInterface;
}

void GraphicsDeviceInterface::destroy() {
    VULKAN_ENSURE_SUCCESS_EXPR(
        device.waitIdle(), "Can't wait for device idle:"
    );

    for (FrameData& frameData : frameDatas) {
        device.destroySemaphore(frameData.isImageAvailable);
        device.destroySemaphore(frameData.isRenderingFinished);
        device.destroyFence(frameData.isRenderingInFlight);
        frameData.sceneDataBuffer.destroyBy(device);
    }

    LLOG_INFO << "Destroyed semaphore and fences";

    cleanupSwapchain();
    LLOG_INFO << "Destroyed swapchain";
    graphics::destroy(device, samplers);
    LLOG_INFO << "Destroyed sampler";
    device.destroyCommandPool(commandPool);
    LLOG_INFO << "Destroyed command pool";
    pipeline.destroyBy(device);
    LLOG_INFO << "Destroyed material pipeline";
    device.destroyRenderPass(renderPass);
    LLOG_INFO << "Destroyed renderpass";

    device.destroy();
    LLOG_INFO << "Destroyed device";

    if (Validation::shouldEnableValidationLayers)
        instance.destroyDebugUtilsMessengerEXT(debugUtilsMessenger);

    instance.destroySurfaceKHR(surface);
    instance.destroy();
    SDL_DestroyWindow(window);
    LLOG_INFO << "Destroyed window";
    SDL_Quit();
}

void GraphicsDeviceInterface::handleEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_EVENT_WINDOW_RESIZED:
            handleWindowResize(event.window.data1, event.window.data2);
            break;
    }
}

void GraphicsDeviceInterface::recreateSwapchain() {
    VULKAN_ENSURE_SUCCESS_EXPR(
        device.waitIdle(), "Can't wait for device to idle:"
    );
    cleanupSwapchain();
    ASSERT(
        !swapchain.has_value(),
        "Trying to recreate swapchain before cleaning up the previous "
        "swapchain"
    );
    swapchain = createSwapchain();
    LLOG_INFO << "Swapchain recreated";
}

void GraphicsDeviceInterface::cleanupSwapchain() {
    ASSERT(swapchain.has_value(), "Swapchain does not exist to clean up");
    destroy(swapchain.value());
    swapchain = std::nullopt;
}

void GraphicsDeviceInterface::handleWindowResize(
    [[maybe_unused]] int _width, [[maybe_unused]] int _height
) {
    recreateSwapchain();
}
}  // namespace graphics

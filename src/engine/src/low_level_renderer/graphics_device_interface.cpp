#include "low_level_renderer/graphics_device_interface.h"

#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <set>
#include <vector>

#include "file_system/file.h"
#include "logger/assert.h"
#include "low_level_renderer/vertex_buffer.h"
#include "private/graphics_device_helper.h"
#include "private/helpful_defines.h"
#include "private/queue_family.h"
#include "private/swapchain.h"
#include "private/validation.h"

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
        GraphicsHelper::getInstanceExtensions();
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
        GraphicsHelper::getBestPhysicalDevice(instance, surface);
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

    const vk::PhysicalDeviceFeatures deviceFeatures;
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

std::tuple<vk::SwapchainKHR, vk::Format, vk::Extent2D> init_createSwapchain(
    SDL_Window* window,
    const vk::PhysicalDevice& physicalDevice,
    const vk::Device& device,
    const vk::SurfaceKHR& surface,
    const QueueFamilyIndices& queueFamily
) {
    const SwapchainSupportDetails swapchainSupport =
        Swapchain::querySwapChainSupport(physicalDevice, surface);
    const vk::SurfaceFormatKHR surfaceFormat =
        Swapchain::chooseSwapSurfaceFormat(swapchainSupport.formats);
    const vk::PresentModeKHR presentMode =
        Swapchain::chooseSwapPresentMode(swapchainSupport.presentModes);
    const vk::Extent2D extent =
        Swapchain::chooseSwapExtent(swapchainSupport.capabilities, window);
    uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;

    if (swapchainSupport.capabilities.maxImageCount > 0)
        imageCount =
            std::min(imageCount, swapchainSupport.capabilities.maxImageCount);

    const bool shouldUseExclusiveSharingMode =
        queueFamily.presentFamily.value() == queueFamily.graphicsFamily.value();
    const uint32_t queueFamilyIndices[] = {
        queueFamily.presentFamily.value(), queueFamily.graphicsFamily.value()
    };
    const vk::SharingMode sharingMode = shouldUseExclusiveSharingMode
                                            ? vk::SharingMode::eExclusive
                                            : vk::SharingMode::eConcurrent;
    const vk::SwapchainCreateInfoKHR swapchainCreateInfo(
        {},
        surface,
        imageCount,
        surfaceFormat.format,
        surfaceFormat.colorSpace,
        extent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment,
        sharingMode,
        shouldUseExclusiveSharingMode ? 0 : 2,
        shouldUseExclusiveSharingMode ? nullptr : queueFamilyIndices,
        swapchainSupport.capabilities.currentTransform,
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        presentMode,
        true,  // clipped
        nullptr
    );
    const vk::ResultValue<vk::SwapchainKHR> swapchainCreateResult =
        device.createSwapchainKHR(swapchainCreateInfo, nullptr);
    VULKAN_ENSURE_SUCCESS(
        swapchainCreateResult.result, "Can't create swapchain:"
    );
    return std::make_tuple(
        swapchainCreateResult.value, surfaceFormat.format, extent
    );
}

std::vector<vk::ImageView> init_createImageViews(
    const vk::Device& device,
    const std::vector<vk::Image>& swapchainImages,
    const vk::Format swapchainImageFormat
) {
    std::vector<vk::ImageView> swapchainImageViews(swapchainImages.size());

    for (unsigned int i = 0; i < swapchainImages.size(); i++) {
        vk::ImageSubresourceRange subResourceRange(
            vk::ImageAspectFlagBits::eColor,
            0,  // base mip level
            1,  // mip level count
            0,  // base array layer (we only use one layer)
            1   // layer count
        );
        vk::ImageViewCreateInfo createInfo(
            {},
            swapchainImages[i],
            vk::ImageViewType::e2D,
            swapchainImageFormat,
            vk::ComponentMapping(),
            subResourceRange
        );
        const vk::ResultValue<vk::ImageView> swapchainImageCreateResult =
            device.createImageView(createInfo);
        VULKAN_ENSURE_SUCCESS(
            swapchainImageCreateResult.result,
            "Can't create swapchain image view:"
        );
        swapchainImageViews[i] = swapchainImageCreateResult.value;
    }

    return swapchainImageViews;
}

vk::ShaderModule createShaderModule(
    const vk::Device& device, const std::vector<char>& code
) {
    vk::ShaderModuleCreateInfo createInfo(
        {}, code.size(), reinterpret_cast<const uint32_t*>(code.data())
    );
    vk::ResultValue<vk::ShaderModule> shaderModule =
        device.createShaderModule(createInfo);
    VULKAN_ENSURE_SUCCESS(shaderModule.result, "Can't create shader");
    return shaderModule.value;
}

vk::RenderPass init_createRenderPass(
    const vk::Device& device, vk::Format swapchainImageFormat
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
        vk::ImageLayout::ePresentSrcKHR
    );
    const vk::AttachmentReference colorAttachmentReference(
        0,  // index of attachment
        vk::ImageLayout::eColorAttachmentOptimal
    );
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
        {},
        vk::AccessFlagBits::eColorAttachmentWrite
    );
    const vk::RenderPassCreateInfo renderPassInfo(
        {}, 1, &colorAttachment, 1, &subpassDescription, 1, &dependency
    );
    const vk::ResultValue<vk::RenderPass> renderPassCreation =
        device.createRenderPass(renderPassInfo);
    VULKAN_ENSURE_SUCCESS(
        renderPassCreation.result, "Can't create renderpass:"
    );
    return renderPassCreation.value;
}

vk::PipelineLayout init_createPipelineLayout(
    const vk::Device& device, const vk::DescriptorSetLayout& descriptorSetLayout
) {
    // empty pipeline layout for now
    const vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
        {}, 1, &descriptorSetLayout, 0, nullptr
    );
    const vk::ResultValue<vk::PipelineLayout> pipelineLayoutCreation =
        device.createPipelineLayout(pipelineLayoutInfo);
    VULKAN_ENSURE_SUCCESS(
        pipelineLayoutCreation.result, "Can't create pipeline layout:"
    );
    return pipelineLayoutCreation.value;
}

std::tuple<vk::Pipeline, std::vector<vk::ShaderModule>>
init_createGraphicsPipeline(
    const vk::Device& device,
    vk::PipelineLayout layout,
    vk::RenderPass renderPass
) {
    const std::optional<std::vector<char>> vertexShaderCode =
        FileUtilities::readFile("shaders/test_triangle.vert.glsl.spv");
    const std::optional<std::vector<char>> fragmentShaderCode =
        FileUtilities::readFile("shaders/test_triangle.frag.glsl.spv");
    ASSERT(vertexShaderCode.has_value(), "Vertex shader can't be loaded");
    ASSERT(fragmentShaderCode.has_value(), "Fragment shader can't be loaded");
    const vk::ShaderModule vertexShader =
        createShaderModule(device, vertexShaderCode.value());
    const vk::ShaderModule fragmentShader =
        createShaderModule(device, fragmentShaderCode.value());
    const std::vector<vk::ShaderModule> shaderModules = {
        vertexShader, fragmentShader
    };
    const vk::PipelineShaderStageCreateInfo vertexShaderStageInfo(
        {}, vk::ShaderStageFlagBits::eVertex, vertexShader, "main"
    );
    const vk::PipelineShaderStageCreateInfo fragmentShaderStageInfo(
        {}, vk::ShaderStageFlagBits::eFragment, fragmentShader, "main"
    );
    const std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };
    const vk::PipelineDynamicStateCreateInfo dynamicStateInfo(
        {}, static_cast<uint32_t>(dynamicStates.size()), dynamicStates.data()
    );
    const vk::PipelineShaderStageCreateInfo shaderStages[] = {
        vertexShaderStageInfo, fragmentShaderStageInfo
    };
    const auto bindingDescription = Vertex::getBindingDescription();
    const auto attributeDescription = Vertex::getAttributeDescriptions();
    const vk::PipelineVertexInputStateCreateInfo vertexInputStateInfo(
        {},
        1,
        &bindingDescription,
        static_cast<uint32_t>(attributeDescription.size()),
        attributeDescription.data()
    );
    const vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo(
        {},
        vk::PrimitiveTopology::eTriangleList,
        vk::False  // primitive restart
    );
    const vk::PipelineViewportStateCreateInfo viewportStateInfo(
        {}, 1, nullptr, 1, nullptr
    );
    const vk::PipelineRasterizationStateCreateInfo rasterizerCreateInfo(
        {},
        vk::False,  // depth clamp enable. only useful for shadow mapping
        vk::False,  // rasterizerDiscardEnable
        vk::PolygonMode::eFill,  // fill polygon with fragments
        vk::CullModeFlagBits::eBack,
        vk::FrontFace::eCounterClockwise,
        vk::False,  // depth bias, probably useful for shadow mapping
        0.0f,
        0.0f,
        0.0f,
        1.0f  // line width
    );
    const vk::PipelineMultisampleStateCreateInfo multisamplingInfo(
        {},
        vk::SampleCountFlagBits::e1,
        vk::False,
        1.0f,
        nullptr,
        vk::False,
        vk::False
    );
    const vk::PipelineColorBlendAttachmentState colorBlendAttachment(
        vk::True,  // enable blend
        vk::BlendFactor::eSrcAlpha,
        vk::BlendFactor::eOneMinusSrcAlpha,
        vk::BlendOp::eAdd,
        vk::BlendFactor::eOne,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    );
    const std::array<float, 4> colorBlendingConstants = {
        0.0f, 0.0f, 0.0f, 0.0f
    };
    const vk::PipelineColorBlendStateCreateInfo colorBlendingInfo(
        {},
        vk::False,
        vk::LogicOp::eCopy,
        1,
        &colorBlendAttachment,
        colorBlendingConstants
    );
    vk::GraphicsPipelineCreateInfo pipelineCreateInfo(
        {},
        2,
        shaderStages,
        &vertexInputStateInfo,
        &inputAssemblyStateInfo,
        nullptr,  // no tesselation viewport
        &viewportStateInfo,
        &rasterizerCreateInfo,
        &multisamplingInfo,
        nullptr,  // no depth stencil
        &colorBlendingInfo,
        &dynamicStateInfo,
        layout,
        renderPass,
        0
    );
    const vk::ResultValue<vk::Pipeline> pipelineCreation =
        device.createGraphicsPipeline(nullptr, pipelineCreateInfo);
    VULKAN_ENSURE_SUCCESS(
        pipelineCreation.result, "Can't create graphics pipeline:"
    );
    return std::make_tuple(pipelineCreation.value, shaderModules);
}

std::vector<vk::Framebuffer> init_createFramebuffer(
    const vk::Device& device,
    const vk::RenderPass& renderPass,
    const vk::Extent2D& swapchainExtent,
    const std::vector<vk::ImageView>& swapchainImageViews
) {
    std::vector<vk::Framebuffer> swapchainFramebuffers(swapchainImageViews.size(
    ));

    for (size_t i = 0; i < swapchainImageViews.size(); i++) {
        ASSERT(
            swapchainImageViews[i],
            "Swapchain image at location " << i << " is null"
        );
        const vk::ImageView attachments[] = {swapchainImageViews[i]};
        const vk::FramebufferCreateInfo framebufferCreateInfo(
            {},
            renderPass,
            1,
            attachments,
            swapchainExtent.width,
            swapchainExtent.height,
            1
        );
        const vk::ResultValue<vk::Framebuffer> framebufferCreation =
            device.createFramebuffer(framebufferCreateInfo);
        VULKAN_ENSURE_SUCCESS(
            framebufferCreation.result, "Can't create framebuffer:"
        );
        swapchainFramebuffers[i] = framebufferCreation.value;
    }

    return swapchainFramebuffers;
}

vk::CommandPool init_createCommandPool(
    const vk::Device& device, const QueueFamilyIndices queueFamilies
) {
    ASSERT(
        queueFamilies.graphicsFamily.has_value(),
        "Calling create command pool on incomplete family indices"
    );
    vk::CommandPoolCreateInfo poolInfo(
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

vk::DescriptorSetLayout init_createDescriptorSetLayout(const vk::Device& device
) {
    const vk::DescriptorSetLayoutBinding uboLayoutBinding(
        0,  // binding
        vk::DescriptorType::eUniformBuffer,
        1,  // descriptor count
        vk::ShaderStageFlagBits::eVertex,
        nullptr  // immutable sampler pointer
    );
    const vk::DescriptorSetLayoutCreateInfo layoutInfo(
        {},
        1,  // binding count
        &uboLayoutBinding
    );
    vk::ResultValue<vk::DescriptorSetLayout> descriptorSetLayout =
        device.createDescriptorSetLayout(layoutInfo);
    VULKAN_ENSURE_SUCCESS(
        descriptorSetLayout.result, "Can't create descriptor set layout:"
    );
    return descriptorSetLayout.value;
}

std::vector<UniformBuffer<ModelViewProjection>> init_createUniformBuffers(
    const vk::Device& device,
    const vk::PhysicalDevice& physicalDevice,
    const uint32_t numberOfFrames
) {
    std::vector<UniformBuffer<ModelViewProjection>> result;
    result.reserve(MAX_FRAMES_IN_FLIGHT);

    for (uint32_t _ = 0; _ < numberOfFrames; _++) {
        result.push_back(
            UniformBuffer<ModelViewProjection>::create(device, physicalDevice)
        );
    }

    return result;
}

vk::DescriptorPool init_createDescriptorPool(const vk::Device& device) {
    const vk::DescriptorPoolSize poolSize(
        vk::DescriptorType::eUniformBuffer,
        static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)
    );
    const vk::DescriptorPoolCreateInfo poolInfo(
        {}, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT), 1, &poolSize
    );
    const vk::ResultValue<vk::DescriptorPool> descriptorPoolCreation =
        device.createDescriptorPool(poolInfo);
    VULKAN_ENSURE_SUCCESS(
        descriptorPoolCreation.result, "Can't create descriptor pool:"
    );
    return descriptorPoolCreation.value;
}

std::vector<vk::DescriptorSet> init_createDescriptorSets(
    const vk::Device& device,
    const vk::DescriptorPool& descriptorPool,
    const vk::DescriptorSetLayout& setLayout,
    const std::vector<UniformBuffer<ModelViewProjection>>& uniformBuffers,
    uint32_t numberOfSets
) {
    const std::vector<vk::DescriptorSetLayout> setLayouts(
        MAX_FRAMES_IN_FLIGHT, setLayout
    );
    const vk::DescriptorSetAllocateInfo allocateInfo(
        descriptorPool, numberOfSets, setLayouts.data()
    );
    const vk::ResultValue<std::vector<vk::DescriptorSet>>
        descriptorSetCreation = device.allocateDescriptorSets(allocateInfo);
    VULKAN_ENSURE_SUCCESS(
        descriptorSetCreation.result, "Can't create descriptor sets"
    );
    std::vector<vk::DescriptorSet> descriptorSets = descriptorSetCreation.value;
    for (size_t i = 0; i < numberOfSets; i++) {
        const vk::DescriptorBufferInfo bufferInfo =
            uniformBuffers[i].getDescriptorBufferInfo();
        const vk::WriteDescriptorSet write(
            descriptorSets[i],
            0,
            0,
            1,
            vk::DescriptorType::eUniformBuffer,
            nullptr,
            &bufferInfo,
            nullptr
        );
        device.updateDescriptorSets(1, &write, 0, nullptr);
    }
    return descriptorSetCreation.value;
}

}  // namespace

GraphicsDeviceInterface::GraphicsDeviceInterface(
    SDL_Window* window,
    vk::Instance instance,
    vk::DebugUtilsMessengerEXT debugUtilsMessenger,
    vk::SurfaceKHR surface,
    vk::Device device,
    vk::PhysicalDevice physicalDevice,
    vk::Queue graphicsQueue,
    vk::Queue presentQueue,
    vk::SwapchainKHR swapchain,
    std::vector<vk::Image> swapchainImages,
    std::vector<vk::ImageView> swapchainImageViews,
    vk::Format swapchainImageFormat,
    vk::Extent2D swapchainExtent,
    vk::RenderPass renderPass,
    vk::DescriptorSetLayout descriptorSetLayout,
    vk::DescriptorPool descriptorPool,
    std::vector<vk::DescriptorSet> descriptorSets,
    vk::PipelineLayout pipelineLayout,
    vk::Pipeline pipeline,
    std::vector<vk::ShaderModule> shaderModules,
    std::vector<vk::Framebuffer> swapchainFramebuffers,
    vk::CommandPool commandPool,
    std::vector<vk::CommandBuffer> commandBuffers,
    std::vector<vk::Semaphore> isImageAvailable,
    std::vector<vk::Semaphore> isRenderingFinished,
    std::vector<vk::Fence> isRenderingInFlight,
    std::vector<UniformBuffer<ModelViewProjection>> uniformBuffers,
    VertexBuffer vertexBuffer
) :
    window(window),
    instance(instance),
    debugUtilsMessenger(debugUtilsMessenger),
    surface(surface),
    device(device),
    physicalDevice(physicalDevice),
    graphicsQueue(graphicsQueue),
    presentQueue(presentQueue),
    swapchain(swapchain),
    swapchainImages(swapchainImages),
    swapchainImageViews(swapchainImageViews),
    swapchainImageFormat(swapchainImageFormat),
    swapchainExtent(swapchainExtent),
    renderPass(renderPass),
    descriptorSetLayout(descriptorSetLayout),
    descriptorPool(descriptorPool),
    descriptorSets(descriptorSets),
    pipelineLayout(pipelineLayout),
    pipeline(pipeline),
    shaderModules(shaderModules),
    swapchainFramebuffers(swapchainFramebuffers),
    commandPool(commandPool),
    commandBuffers(commandBuffers),
    isImageAvailable(isImageAvailable),
    isRenderingFinished(isRenderingFinished),
    isRenderingInFlight(isRenderingInFlight),
    uniformBuffers(uniformBuffers),
    vertexBuffer(vertexBuffer),
    currentFrame(0) {}

GraphicsDeviceInterface GraphicsDeviceInterface::createGraphicsDevice() {
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
    vk::Instance instance;
    vk::DebugUtilsMessengerEXT debugUtilsMessenger;
    std::tie(instance, debugUtilsMessenger) = init_createInstance();
    vk::SurfaceKHR surface = init_createSurface(window, instance);
    vk::PhysicalDevice physicalDevice =
        init_createPhysicalDevice(instance, surface);
    QueueFamilyIndices queueFamily =
        QueueFamilyIndices::findQueueFamilies(physicalDevice, surface);
    vk::Device device = init_createLogicalDevice(physicalDevice, queueFamily);
    vk::Queue graphicsQueue =
        device.getQueue(queueFamily.graphicsFamily.value(), 0);
    vk::Queue presentQueue =
        device.getQueue(queueFamily.presentFamily.value(), 0);
    {
        LLOG_INFO << "Available Extensions:";
        const auto instanceExtensionProps =
            vk::enumerateInstanceExtensionProperties();
        VULKAN_ENSURE_SUCCESS(
            instanceExtensionProps.result,
            "Can't get instance extension properties"
        );

        for (const auto& extension : instanceExtensionProps.value)
            LLOG_INFO << "\t" << extension.extensionName;
    }

    vk::SwapchainKHR swapchain;
    vk::Format swapchainImageFormat;
    vk::Extent2D swapchainExtent;
    std::tie(swapchain, swapchainImageFormat, swapchainExtent) =
        init_createSwapchain(
            window, physicalDevice, device, surface, queueFamily
        );
    LLOG_INFO << "Swapchain created with format "
              << to_string(swapchainImageFormat) << " and extent "
              << swapchainExtent.width << " x " << swapchainExtent.height;
    const auto swapchainImagesGet = device.getSwapchainImagesKHR(swapchain);
    VULKAN_ENSURE_SUCCESS(
        swapchainImagesGet.result, "Can't get swapchain images:"
    );
    std::vector<vk::Image> swapchainImages = swapchainImagesGet.value;
    std::vector<vk::ImageView> swapchainImageViews =
        init_createImageViews(device, swapchainImages, swapchainImageFormat);
    vk::DescriptorSetLayout descriptorSetLayout =
        init_createDescriptorSetLayout(device);
    std::vector<UniformBuffer<ModelViewProjection>> uniformBuffers =
        init_createUniformBuffers(device, physicalDevice, MAX_FRAMES_IN_FLIGHT);
    vk::DescriptorPool descriptorPool = init_createDescriptorPool(device);
    std::vector<vk::DescriptorSet> descriptorSets = init_createDescriptorSets(
        device,
        descriptorPool,
        descriptorSetLayout,
        uniformBuffers,
        MAX_FRAMES_IN_FLIGHT
    );
    vk::PipelineLayout pipelineLayout =
        init_createPipelineLayout(device, descriptorSetLayout);
    vk::RenderPass renderPass =
        init_createRenderPass(device, swapchainImageFormat);
    vk::Pipeline pipeline;
    std::vector<vk::ShaderModule> shaderModules;
    std::tie(pipeline, shaderModules) =
        init_createGraphicsPipeline(device, pipelineLayout, renderPass);
    std::vector<vk::Framebuffer> swapchainFramebuffers = init_createFramebuffer(
        device, renderPass, swapchainExtent, swapchainImageViews
    );
    vk::CommandPool commandPool = init_createCommandPool(device, queueFamily);
    VertexBuffer vertexBuffer = VertexBuffer::create(
        device, physicalDevice, commandPool, graphicsQueue
    );
    std::vector<vk::CommandBuffer> commandBuffers =
        init_createCommandBuffers(device, commandPool, MAX_FRAMES_IN_FLIGHT);
    std::vector<vk::Semaphore> isImageAvailable;
    std::vector<vk::Semaphore> isRenderingFinished;
    std::vector<vk::Fence> isRenderingInFlight;
    std::tie(isImageAvailable, isRenderingFinished, isRenderingInFlight) =
        init_createSyncObjects(device, MAX_FRAMES_IN_FLIGHT);
    return GraphicsDeviceInterface(
        window,
        instance,
        debugUtilsMessenger,
        surface,
        device,
        physicalDevice,
        graphicsQueue,
        presentQueue,
        swapchain,
        swapchainImages,
        swapchainImageViews,
        swapchainImageFormat,
        swapchainExtent,
        renderPass,
        descriptorSetLayout,
        descriptorPool,
        descriptorSets,
        pipelineLayout,
        pipeline,
        shaderModules,
        swapchainFramebuffers,
        commandPool,
        commandBuffers,
        isImageAvailable,
        isRenderingFinished,
        isRenderingInFlight,
        uniformBuffers,
        vertexBuffer
    );
}

GraphicsDeviceInterface::~GraphicsDeviceInterface() {
    VULKAN_ENSURE_SUCCESS_EXPR(
        device.waitIdle(), "Can't wait for device idle:"
    );

    for (const vk::Semaphore& semaphore : isImageAvailable)
        device.destroySemaphore(semaphore);

    for (const vk::Semaphore& semaphore : isRenderingFinished)
        device.destroySemaphore(semaphore);

    for (const vk::Fence& fence : isRenderingInFlight)
        device.destroyFence(fence);

    cleanupSwapchain();
    vertexBuffer.destroyBy(device);
    device.destroyCommandPool(commandPool);
    device.destroyDescriptorSetLayout(descriptorSetLayout);
    device.destroyDescriptorPool(descriptorPool);
    device.destroyPipelineLayout(pipelineLayout);
    device.destroyRenderPass(renderPass);

    for (UniformBuffer<ModelViewProjection>& uniformBuffer : uniformBuffers)
        uniformBuffer.destroyBy(device);

    for (const vk::ShaderModule& shaderModule : shaderModules)
        device.destroyShaderModule(shaderModule);

    device.destroyPipeline(pipeline);
    device.destroy();

    if (Validation::shouldEnableValidationLayers)
        instance.destroyDebugUtilsMessengerEXT(debugUtilsMessenger);

    instance.destroySurfaceKHR(surface);
    instance.destroy();
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void GraphicsDeviceInterface::recordCommandBuffer(
    vk::CommandBuffer buffer, uint32_t imageIndex
) {
    vk::CommandBufferBeginInfo beginInfo({}, nullptr);
    VULKAN_ENSURE_SUCCESS_EXPR(
        buffer.begin(beginInfo), "Can't begin recording command buffer:"
    );
    vk::ClearValue clearColor(vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f));
    vk::RenderPassBeginInfo renderPassInfo(
        renderPass,
        swapchainFramebuffers[imageIndex],
        vk::Rect2D(vk::Offset2D{0, 0}, swapchainExtent),
        1,
        &clearColor
    );
    buffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
    vertexBuffer.bind(buffer);
    vk::Viewport viewport(
        0.0f,
        0.0f,
        static_cast<float>(swapchainExtent.width),
        static_cast<float>(swapchainExtent.height),
        0.0f,
        1.0f
    );
    buffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        pipelineLayout,
        0,
        1,
        &descriptorSets[currentFrame],
        0,
        nullptr
    );
    buffer.setViewport(0, 1, &viewport);
    vk::Rect2D scissor(vk::Offset2D(0.0f, 0.0f), swapchainExtent);
    buffer.setScissor(0, 1, &scissor);
    vertexBuffer.draw(buffer);
    buffer.endRenderPass();
    VULKAN_ENSURE_SUCCESS_EXPR(
        buffer.end(), "Can't end recording command buffer:"
    );
}

bool GraphicsDeviceInterface::drawFrame() {
    const uint64_t no_time_limit = std::numeric_limits<uint64_t>::max();
    VULKAN_ENSURE_SUCCESS_EXPR(
        device.waitForFences(
            1, &isRenderingInFlight[currentFrame], vk::True, no_time_limit
        ),
        "Can't wait for previous frame rendering:"
    );
    const vk::ResultValue<uint32_t> imageIndex = device.acquireNextImageKHR(
        swapchain, no_time_limit, isImageAvailable[currentFrame], nullptr
    );

    switch (imageIndex.result) {
        case vk::Result::eErrorOutOfDateKHR:
            LLOG_INFO << "Out of date KHR";
            recreateSwapchain();
            return true;

        case vk::Result::eSuccess:
        case vk::Result::eSuboptimalKHR: break;

        default: return false;
    }

    VULKAN_ENSURE_SUCCESS_EXPR(
        device.resetFences(1, &isRenderingInFlight[currentFrame]),
        "Can't reset fence for render:"
    );
    commandBuffers[currentFrame].reset();
    uniformBuffers[currentFrame].update(getCurrentFrameMVP());
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex.value);
    const vk::PipelineStageFlags waitStage =
        vk::PipelineStageFlagBits::eColorAttachmentOutput;
    const vk::SubmitInfo submitInfo(
        1,
        &isImageAvailable[currentFrame],
        &waitStage,
        1,
        &commandBuffers[currentFrame],
        1,
        &isRenderingFinished[currentFrame]
    );
    VULKAN_ENSURE_SUCCESS_EXPR(
        graphicsQueue.submit(1, &submitInfo, isRenderingInFlight[currentFrame]),
        "Can't submit graphics queue:"
    );
    vk::PresentInfoKHR presentInfo(
        1,
        &isRenderingFinished[currentFrame],
        1,
        &swapchain,
        &imageIndex.value,
        nullptr
    );

    switch (presentQueue.presentKHR(presentInfo)) {
        case vk::Result::eErrorOutOfDateKHR:
        case vk::Result::eSuboptimalKHR:     recreateSwapchain();

        case vk::Result::eSuccess: break;

        default: return false;
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    return true;
}

void GraphicsDeviceInterface::handleEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_EVENT_WINDOW_RESIZED:
            handleWindowResize(event.window.data1, event.window.data2);
            break;
    }
}

ModelViewProjection GraphicsDeviceInterface::getCurrentFrameMVP() const {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(
                     currentTime - startTime
    )
                     .count();

    ModelViewProjection mvp{
        .model = glm::rotate(
            glm::mat4(1.0f),
            time * glm::radians(90.0f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        ),
        .view = glm::lookAt(
            glm::vec3(2.0f, 2.0f, 2.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        ),
        .proj = glm::perspective(
            glm::radians(45.0f),
            swapchainExtent.width / (float)swapchainExtent.height,
            0.1f,
            10.0f
        )
    };
    // accounts for difference between openGL and Vulkan clip space
    mvp.proj[1][1] *= -1;

    return mvp;
}

void GraphicsDeviceInterface::recreateSwapchain() {
    VULKAN_ENSURE_SUCCESS_EXPR(
        device.waitIdle(), "Can't wait for device to idle:"
    );
    cleanupSwapchain();
    QueueFamilyIndices queueFamily =
        QueueFamilyIndices::findQueueFamilies(physicalDevice, surface);
    std::tie(swapchain, swapchainImageFormat, swapchainExtent) =
        init_createSwapchain(
            window, physicalDevice, device, surface, queueFamily
        );
    const auto swapchainImagesGet = device.getSwapchainImagesKHR(swapchain);
    VULKAN_ENSURE_SUCCESS(
        swapchainImagesGet.result, "Can't get swapchain images:"
    );
    swapchainImages = swapchainImagesGet.value;
    swapchainImageViews =
        init_createImageViews(device, swapchainImages, swapchainImageFormat);
    swapchainFramebuffers = init_createFramebuffer(
        device, renderPass, swapchainExtent, swapchainImageViews
    );
    LLOG_INFO << "Swapchain recreated";
}

void GraphicsDeviceInterface::cleanupSwapchain() {
    for (const vk::Framebuffer& framebuffer : swapchainFramebuffers)
        device.destroyFramebuffer(framebuffer);

    for (const vk::ImageView& imageView : swapchainImageViews)
        device.destroyImageView(imageView);

    device.destroySwapchainKHR(std::move(swapchain));
}

void GraphicsDeviceInterface::handleWindowResize(
    [[maybe_unused]] int _width, [[maybe_unused]] int _height
) {
    recreateSwapchain();
}

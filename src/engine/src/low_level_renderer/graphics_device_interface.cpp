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
        vk::ImageLayout::ePresentSrcKHR
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

vk::PipelineLayout init_createPipelineLayout(
    const vk::Device& device, const vk::DescriptorSetLayout& descriptorSetLayout
) {
    // We're pushing the transform to the vertex shader
    const vk::PushConstantRange pushConstantRange(
        vk::ShaderStageFlagBits::eVertex, 0, sizeof(ModelViewProjection)
    );
    const vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
        {}, 1, &descriptorSetLayout, 1, &pushConstantRange
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
    const vk::PipelineDepthStencilStateCreateInfo depthStencilState(
        {},
        vk::True,  // enable depth test
        vk::True,  // enable depth write
        vk::CompareOp::eLess,
        vk::False,  // disable depth bounds test
        vk::False,  // disable stencil test
        {},         // stencil front op state
        {},         // stencil back op state
        {},         // min depth bound
        {}          // max depth bound
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
        &depthStencilState,
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
    const vk::DescriptorSetLayoutBinding textureSamplerBinding(
        1,  // binding
        vk::DescriptorType::eCombinedImageSampler,
        1,  // descriptor count
        vk::ShaderStageFlagBits::eFragment,
        nullptr  // immutable sampler pointer
    );
    const std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {
        uboLayoutBinding, textureSamplerBinding
    };
    const vk::DescriptorSetLayoutCreateInfo layoutInfo(
        {},
        static_cast<uint32_t>(bindings.size()),  // binding count
        bindings.data()
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
    const std::array<vk::DescriptorPoolSize, 2> poolSizes = {
        vk::DescriptorPoolSize(
            vk::DescriptorType::eUniformBuffer,
            static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)
        ),
        vk::DescriptorPoolSize(
            vk::DescriptorType::eCombinedImageSampler,
            static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)
        ),
    };
    const vk::DescriptorPoolCreateInfo poolInfo(
        {},
        static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
        static_cast<uint32_t>(poolSizes.size()),
        poolSizes.data()
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
    uint32_t numberOfSets,
    const Texture& texture,
    const Sampler& sampler
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
        const vk::DescriptorImageInfo imageInfo =
            texture.getDescriptorImageInfo(sampler);
        const std::array<vk::WriteDescriptorSet, 2> writes = {
            vk::WriteDescriptorSet(
                descriptorSets[i],
                0,
                0,
                1,
                vk::DescriptorType::eUniformBuffer,
                nullptr,
                &bufferInfo,
                nullptr
            ),
            vk::WriteDescriptorSet(
                descriptorSets[i],
                1,
                0,
                1,
                vk::DescriptorType::eCombinedImageSampler,
                &imageInfo,
                nullptr,
                nullptr
            ),
        };
        device.updateDescriptorSets(
            static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr
        );
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
    vk::RenderPass renderPass,
    vk::DescriptorSetLayout descriptorSetLayout,
    vk::DescriptorPool descriptorPool,
    std::vector<vk::DescriptorSet> descriptorSets,
    vk::PipelineLayout pipelineLayout,
    vk::Pipeline pipeline,
    std::vector<vk::ShaderModule> shaderModules,
    vk::CommandPool commandPool,
    std::vector<vk::CommandBuffer> commandBuffers,
    std::vector<vk::Semaphore> isImageAvailable,
    std::vector<vk::Semaphore> isRenderingFinished,
    std::vector<vk::Fence> isRenderingInFlight,
    std::vector<UniformBuffer<ModelViewProjection>> uniformBuffers,
    Mesh mesh,
    Sampler sampler
) :
    window(window),
    instance(instance),
    debugUtilsMessenger(debugUtilsMessenger),
    surface(surface),
    device(device),
    physicalDevice(physicalDevice),
    graphicsQueue(graphicsQueue),
    presentQueue(presentQueue),
    renderPass(renderPass),
    descriptorSetLayout(descriptorSetLayout),
    descriptorPool(descriptorPool),
    descriptorSets(descriptorSets),
    pipelineLayout(pipelineLayout),
    pipeline(pipeline),
    shaderModules(shaderModules),
    commandPool(commandPool),
    commandBuffers(commandBuffers),
    isImageAvailable(isImageAvailable),
    isRenderingFinished(isRenderingFinished),
    isRenderingInFlight(isRenderingInFlight),
    uniformBuffers(uniformBuffers),
    mesh(mesh),
    sampler(sampler),
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
    const vk::DescriptorSetLayout descriptorSetLayout =
        init_createDescriptorSetLayout(device);
    const std::vector<UniformBuffer<ModelViewProjection>> uniformBuffers =
        init_createUniformBuffers(device, physicalDevice, MAX_FRAMES_IN_FLIGHT);
    const vk::DescriptorPool descriptorPool = init_createDescriptorPool(device);
    const std::vector<vk::CommandBuffer> commandBuffers =
        init_createCommandBuffers(device, commandPool, MAX_FRAMES_IN_FLIGHT);
    LLOG_INFO << "Created command pool and buffers";

    const Mesh mesh = Mesh::load(
        device,
        physicalDevice,
        commandPool,
        graphicsQueue,
        "models/sword.obj",
        "textures/swordAlbedo.jpg"
    );
    const Sampler sampler = Sampler::create(device, physicalDevice);
    const std::vector<vk::DescriptorSet> descriptorSets =
        init_createDescriptorSets(
            device,
            descriptorPool,
            descriptorSetLayout,
            uniformBuffers,
            MAX_FRAMES_IN_FLIGHT,
            mesh.albedo,
            sampler
        );
    const vk::SurfaceFormatKHR swapchainColorFormat =
        Swapchain::getSuitableColorAttachmentFormat(physicalDevice, surface);
    LLOG_INFO << "Created descriptor pool and sets";
    const vk::PipelineLayout pipelineLayout =
        init_createPipelineLayout(device, descriptorSetLayout);
    const vk::RenderPass renderPass = init_createRenderPass(
        device,
        swapchainColorFormat.format,
        Swapchain::getSuitableDepthAttachmentFormat(physicalDevice)
    );
    const auto [pipeline, shaderModules] =
        init_createGraphicsPipeline(device, pipelineLayout, renderPass);
    LLOG_INFO << "Created pipeline layout, renderpass, and pipeline";
    const auto [isImageAvailable, isRenderingFinished, isRenderingInFlight] =
        init_createSyncObjects(device, MAX_FRAMES_IN_FLIGHT);
    LLOG_INFO << "Created semaphore and fences";

    GraphicsDeviceInterface deviceInterface(
        window,
        instance,
        debugUtilsMessenger,
        surface,
        device,
        physicalDevice,
        graphicsQueue,
        presentQueue,
        renderPass,
        descriptorSetLayout,
        descriptorPool,
        descriptorSets,
        pipelineLayout,
        pipeline,
        shaderModules,
        commandPool,
        commandBuffers,
        isImageAvailable,
        isRenderingFinished,
        isRenderingInFlight,
        uniformBuffers,
        mesh,
        sampler
    );

    deviceInterface.swapchain = deviceInterface.createSwapchain();

    return deviceInterface;
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
    LLOG_INFO << "Destroyed semaphore and fences";

    cleanupSwapchain();
    LLOG_INFO << "Destroyed swapchain";
    sampler.destroyBy(device);
    LLOG_INFO << "Destroyed sampler";
    mesh.destroyBy(device);
    LLOG_INFO << "Destroyed mesh";
    device.destroyCommandPool(commandPool);
    LLOG_INFO << "Destroyed command pool";
    device.destroyDescriptorSetLayout(descriptorSetLayout);
    device.destroyDescriptorPool(descriptorPool);
    LLOG_INFO << "Destroyed descriptor pool";
    device.destroyPipelineLayout(pipelineLayout);
    device.destroyRenderPass(renderPass);

    for (UniformBuffer<ModelViewProjection>& uniformBuffer : uniformBuffers)
        uniformBuffer.destroyBy(device);

    for (const vk::ShaderModule& shaderModule : shaderModules)
        device.destroyShaderModule(shaderModule);
    LLOG_INFO << "Destroyed shaders";

    device.destroyPipeline(pipeline);
    LLOG_INFO << "Destroyed pipeline";

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

void GraphicsDeviceInterface::recordCommandBuffer(
    vk::CommandBuffer buffer, uint32_t imageIndex
) {
    ASSERT(swapchain, "Attempt to record command buffer");
    vk::CommandBufferBeginInfo beginInfo({}, nullptr);
    VULKAN_ENSURE_SUCCESS_EXPR(
        buffer.begin(beginInfo), "Can't begin recording command buffer:"
    );
    const std::array<vk::ClearValue, 2> clearColors{
        vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f),
        vk::ClearColorValue(1.0f, 0.0f, 0.0f, 0.0f)
    };
    vk::RenderPassBeginInfo renderPassInfo(
        renderPass,
        swapchain->framebuffers[imageIndex],
        vk::Rect2D(vk::Offset2D{0, 0}, swapchain->extent),
        static_cast<uint32_t>(clearColors.size()),
        clearColors.data()
    );
    buffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
    ModelViewProjection mvp = getCurrentFrameMVP();
    buffer.pushConstants(
        pipelineLayout,
        vk::ShaderStageFlagBits::eVertex,
        0,
        sizeof(ModelViewProjection),
        &mvp
    );
    mesh.bind(buffer);
    vk::Viewport viewport(
        0.0f,
        0.0f,
        static_cast<float>(swapchain->extent.width),
        static_cast<float>(swapchain->extent.height),
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
    vk::Rect2D scissor(vk::Offset2D(0.0f, 0.0f), swapchain->extent);
    buffer.setScissor(0, 1, &scissor);
    mesh.draw(buffer);
    buffer.endRenderPass();
    VULKAN_ENSURE_SUCCESS_EXPR(
        buffer.end(), "Can't end recording command buffer:"
    );
}

bool GraphicsDeviceInterface::drawFrame() {
    ASSERT(swapchain, "Attempt to draw frame without a swapchain");

    const uint64_t no_time_limit = std::numeric_limits<uint64_t>::max();
    VULKAN_ENSURE_SUCCESS_EXPR(
        device.waitForFences(
            1, &isRenderingInFlight[currentFrame], vk::True, no_time_limit
        ),
        "Can't wait for previous frame rendering:"
    );
    const vk::ResultValue<uint32_t> imageIndex = device.acquireNextImageKHR(
        swapchain->swapchain,
        no_time_limit,
        isImageAvailable[currentFrame],
        nullptr
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
        &swapchain->swapchain,
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
    ASSERT(swapchain, "No swapchain exists");

    ModelViewProjection mvp{
        .model = glm::rotate(
            glm::mat4(1.0f),
            time * glm::radians(90.0f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        ),
        .view = glm::lookAt(
            glm::vec3(10.0f, 10.0f, 10.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        ),
        .proj = glm::perspective(
            glm::radians(45.0f),
            swapchain->extent.width / (float)swapchain->extent.height,
            0.1f,
            45.0f
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
    ASSERT(
        !swapchain.has_value(),
        "Trying to recreate swapchain before cleaning up the previous swapchain"
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

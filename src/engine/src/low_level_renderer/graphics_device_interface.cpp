#include <vector>
#include <set>
#include <limits>

#include "logger/assert.h"
#include "file_system/file.h"
#include "low_level_renderer/graphics_device_interface.h"

#include "private/swapchain.h"
#include "private/queue_family.h"
#include "private/graphics_device_helper.h"
#include "private/queue_family.h"
#include "private/validation.h"
#include "vulkan/vulkan_enums.hpp"

const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

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
	ASSERT(!Validation::shouldEnableValidationLayers
		   || Validation::areValidationLayersSupported(),
		   "Validation layers enabled but not supported");
	const vk::InstanceCreateInfo instanceInfo(
		vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
		&appInfo,
		Validation::shouldEnableValidationLayers ? static_cast<uint32_t>
		(Validation::validationLayers.size()) : 0,
		Validation::shouldEnableValidationLayers ? Validation::validationLayers.data() :
		nullptr,
		instanceExtensions.size(),
		instanceExtensions.data()
	);
	vk::Instance instance = vk::createInstance(instanceInfo, nullptr);
	vk::DebugUtilsMessengerEXT debugUtilsMessenger;

	if (Validation::shouldEnableValidationLayers)
		debugUtilsMessenger = Validation::createDebugMessenger(instance);

	return std::make_tuple(instance, debugUtilsMessenger);
}

vk::SurfaceKHR init_createSurface(
	SDL_Window* window,
	const vk::Instance& instance
) {
	VkSurfaceKHR surface;
	bool isSurfaceCreationSuccessful = SDL_Vulkan_CreateSurface(window, instance,
									   nullptr, &surface);
	ASSERT(isSurfaceCreationSuccessful, "Cannot create surface");
	return surface;
}

vk::PhysicalDevice init_createPhysicalDevice(
	const vk::Instance& instance,
	const vk::SurfaceKHR& surface
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
		queueFamily.graphicsFamily.value(),
		queueFamily.presentFamily.value()
	};
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

	for (uint32_t queueFamilyIndex : uniqueQueueFamilies) {
		vk::DeviceQueueCreateInfo deviceQueueCreateInfo({}, queueFamilyIndex, 1,
				&queuePriority);
		queueCreateInfos.push_back(deviceQueueCreateInfo);
	}

	const vk::PhysicalDeviceFeatures deviceFeatures;
	const vk::DeviceCreateInfo deviceCreateInfo(
		{},
		queueCreateInfos.size(),
		queueCreateInfos.data(),
		Validation::shouldEnableValidationLayers ? static_cast<uint32_t>
		(Validation::validationLayers.size()) : 0,
		Validation::shouldEnableValidationLayers ? Validation::validationLayers.data() :
		nullptr,
		static_cast<uint32_t>(deviceExtensions.size()),
		deviceExtensions.data(),
		&deviceFeatures
	);
	vk::Device device;
	vk::Result result = physicalDevice.createDevice(&deviceCreateInfo, nullptr,
						&device);
	ASSERT(result == vk::Result::eSuccess,
		   "Failed to create device with result: " << result);
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
	const vk::SurfaceFormatKHR surfaceFormat = Swapchain::chooseSwapSurfaceFormat(
			swapchainSupport.formats);
	const vk::PresentModeKHR presentMode = Swapchain::chooseSwapPresentMode(
			swapchainSupport.presentModes);
	const vk::Extent2D extent = Swapchain::chooseSwapExtent(
									swapchainSupport.capabilities, window);
	uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;

	if (swapchainSupport.capabilities.maxImageCount > 0)
		imageCount = std::min(imageCount, swapchainSupport.capabilities.maxImageCount);

	const bool shouldUseExclusiveSharingMode = queueFamily.presentFamily.value() ==
			queueFamily.graphicsFamily.value();
	const uint32_t queueFamilyIndices[] = {queueFamily.presentFamily.value(), queueFamily.graphicsFamily.value()};
	const vk::SharingMode sharingMode = shouldUseExclusiveSharingMode ?
										vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent;
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
		true, // clipped
		nullptr
	);
	return std::make_tuple(device.createSwapchainKHR(swapchainCreateInfo, nullptr),
						   surfaceFormat.format, extent);
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
			0, // base mip level
			1, // mip level count
			0, // base array layer (we only use one layer)
			1 // layer count
		);
		vk::ImageViewCreateInfo createInfo(
			{},
			swapchainImages[i],
			vk::ImageViewType::e2D,
			swapchainImageFormat,
			vk::ComponentMapping(),
			subResourceRange
		);
		swapchainImageViews[i] = device.createImageView(createInfo);
	}

	return swapchainImageViews;
}

vk::ShaderModule createShaderModule(const vk::Device& device,
									const std::vector<char>& code) {
	vk::ShaderModuleCreateInfo createInfo(
		{},
		code.size(),
		reinterpret_cast<const uint32_t*>(code.data())
	);
	return device.createShaderModule(createInfo);
}

vk::RenderPass init_createRenderPass(const vk::Device &device,
									 vk::Format swapchainImageFormat) {
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
		0, // index of attachment
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
		{},
		1,
		&colorAttachment,
		1,
		&subpassDescription,
		1,
		&dependency
	);
	return device.createRenderPass(renderPassInfo);
}

vk::PipelineLayout init_createPipelineLayout(const vk::Device& device) {
	// empty pipeline layout for now
	const vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
		{}, 0, nullptr, 0, nullptr
	);
	return device.createPipelineLayout(pipelineLayoutInfo);
}

std::tuple<vk::Pipeline, std::vector<vk::ShaderModule >> init_createGraphicsPipeline(const vk::Device& device, vk::PipelineLayout layout, vk::RenderPass renderPass) {
	const std::optional<std::vector<char >> vertexShaderCode =
		FileUtilities::readFile("shaders/test_triangle.vert.glsl.spv");
	const std::optional<std::vector<char >> fragmentShaderCode =
		FileUtilities::readFile("shaders/test_triangle.frag.glsl.spv");
	ASSERT(vertexShaderCode.has_value(), "Vertex shader can't be loaded");
	ASSERT(fragmentShaderCode.has_value(), "Fragment shader can't be loaded");
	const vk::ShaderModule vertexShader = createShaderModule(device, vertexShaderCode.value());
	const vk::ShaderModule fragmentShader = createShaderModule(device, fragmentShaderCode.value());
	const std::vector<vk::ShaderModule> shaderModules = {
		vertexShader, fragmentShader
	};
	const vk::PipelineShaderStageCreateInfo vertexShaderStageInfo(
		{},
		vk::ShaderStageFlagBits::eVertex,
		vertexShader,
		"main"
	);
	const vk::PipelineShaderStageCreateInfo fragmentShaderStageInfo(
		{},
		vk::ShaderStageFlagBits::eFragment,
		fragmentShader,
		"main"
	);
	const std::vector<vk::DynamicState> dynamicStates = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor,
	};
	const vk::PipelineDynamicStateCreateInfo dynamicStateInfo(
		{},
		static_cast<uint32_t>(dynamicStates.size()),
		dynamicStates.data()
	);
	const vk::PipelineShaderStageCreateInfo shaderStages[]
		= {vertexShaderStageInfo, fragmentShaderStageInfo};
	const vk::PipelineVertexInputStateCreateInfo vertexInputStateInfo(
		{},
		0,
		nullptr,
		0,
		nullptr
	);
	const vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo(
		{},
		vk::PrimitiveTopology::eTriangleList,
		vk::False // primitive restart
	);
	const vk::PipelineViewportStateCreateInfo viewportStateInfo({}, 1, nullptr, 1,
			nullptr);
	const vk::PipelineRasterizationStateCreateInfo rasterizerCreateInfo(
		{},
		vk::False, // depth clamp enable. only useful for shadow mapping
		vk::False, // rasterizerDiscardEnable
		vk::PolygonMode::eFill, // fill polygon with fragments
		vk::CullModeFlagBits::eBack,
		vk::FrontFace::eClockwise,
		vk::False, // depth bias, probably useful for shadow mapping
		0.0f,
		0.0f,
		0.0f,
		1.0f // line width
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
		vk::True, // enable blend
		vk::BlendFactor::eSrcAlpha,
		vk::BlendFactor::eOneMinusSrcAlpha,
		vk::BlendOp::eAdd,
		vk::BlendFactor::eOne,
		vk::BlendFactor::eZero,
		vk::BlendOp::eAdd,
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
		vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
	);
	const std::array<float, 4> colorBlendingConstants = {0.0f, 0.0f, 0.0f, 0.0f};
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
		nullptr, // no tesselation viewport
		&viewportStateInfo,
		&rasterizerCreateInfo,
		&multisamplingInfo,
		nullptr, // no depth stencil
		&colorBlendingInfo,
		&dynamicStateInfo,
		layout,
		renderPass,
		0
	);
	vk::ResultValue<vk::Pipeline> pipeline = device.createGraphicsPipeline(nullptr,
		pipelineCreateInfo);
	ASSERT(pipeline.result == vk::Result::eSuccess,
		   "Can't create graphics pipeline");
	return std::make_tuple(pipeline.value, shaderModules);
}

std::vector<vk::Framebuffer> init_createFramebuffer(
	const vk::Device& device,
	const vk::RenderPass& renderPass,
	const vk::Extent2D& swapchainExtent,
	const std::vector<vk::ImageView>& swapchainImageViews
) {
	std::vector<vk::Framebuffer> swapchainFramebuffers(swapchainImageViews.size());

	for (size_t i = 0; i < swapchainImageViews.size(); i++) {
		ASSERT(swapchainImageViews[i],
			   "Swapchain image at location " << i << " is null");
		vk::ImageView attachments[] = {swapchainImageViews[i]};
		vk::FramebufferCreateInfo framebufferCreateInfo(
			{},
			renderPass,
			1,
			attachments,
			swapchainExtent.width,
			swapchainExtent.height,
			1
		);
		swapchainFramebuffers[i] = device.createFramebuffer(framebufferCreateInfo);
	}

	return swapchainFramebuffers;
}

vk::CommandPool init_createCommandPool(const vk::Device& device,
									   const QueueFamilyIndices queueFamilies) {
	ASSERT(queueFamilies.graphicsFamily.has_value(),
		   "Calling create command pool on incomplete family indices");
	vk::CommandPoolCreateInfo poolInfo(
		vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		queueFamilies.graphicsFamily.value()
	);
	return device.createCommandPool(poolInfo);
}

std::vector<vk::CommandBuffer> init_createCommandBuffers(
	const vk::Device& device,
	const vk::CommandPool& pool,
	const uint32_t num_of_frames
) {
	vk::CommandBufferAllocateInfo allocateInfo(
		pool,
		vk::CommandBufferLevel::ePrimary,
		num_of_frames
	);
	std::vector<vk::CommandBuffer> commandBuffers = device.allocateCommandBuffers(
			allocateInfo);
	ASSERT(commandBuffers.size() == MAX_FRAMES_IN_FLIGHT,
		   "Created more command buffers than specified");
	return commandBuffers;
}

std::tuple<std::vector<vk::Semaphore>, std::vector<vk::Semaphore>, std::vector<vk::Fence >> init_createSyncObjects(
	const vk::Device& device, const uint32_t num_of_frames) {
	std::vector<vk::Semaphore> isImageAvailable(num_of_frames);
	std::vector<vk::Semaphore> isRenderingFinished(num_of_frames);
	std::vector<vk::Fence> isRenderingInFlight(num_of_frames);
	vk::SemaphoreCreateInfo semaphoreCreateInfo;
	vk::FenceCreateInfo fenceCreateInfo(vk::FenceCreateFlagBits::eSignaled);

	for (uint32_t i = 0; i < num_of_frames; i++) {
		isImageAvailable[i] = device.createSemaphore(semaphoreCreateInfo);
		isRenderingFinished[i] = device.createSemaphore(semaphoreCreateInfo);
		isRenderingInFlight[i] = device.createFence(fenceCreateInfo);
	}

	return std::make_tuple(isImageAvailable, isRenderingFinished,
						   isRenderingInFlight);
}
}

GraphicsDeviceInterface::GraphicsDeviceInterface() {
	currentFrame = 0;
	isConstructionSuccessful = true;
	const SDL_InitFlags initFlags = SDL_INIT_VIDEO | SDL_INIT_EVENTS;
	const bool isSDLInitSuccessful = SDL_Init(initFlags);
	ASSERT(isSDLInitSuccessful,
		   "SDL cannot be initialized with flag " << initFlags << " error: " <<
		   SDL_GetError());
	isConstructionSuccessful &= isSDLInitSuccessful;

	if (!isSDLInitSuccessful) return;

	const bool isVulkanLibraryLoadSuccessful = SDL_Vulkan_LoadLibrary(nullptr);
	ASSERT(isVulkanLibraryLoadSuccessful, "Vulkan library cannot be loaded");
	isConstructionSuccessful &= isVulkanLibraryLoadSuccessful;

	if (!isVulkanLibraryLoadSuccessful) return;

	window = SDL_CreateWindow("Liebeskind", 1024, 768, SDL_WINDOW_VULKAN);
	std::tie(instance, debugUtilsMessenger) = init_createInstance();
	surface = init_createSurface(window, instance);
	physicalDevice = init_createPhysicalDevice(instance, surface);
	QueueFamilyIndices queueFamily = QueueFamilyIndices::findQueueFamilies(
										 physicalDevice, surface);
	device = init_createLogicalDevice(physicalDevice, queueFamily);
	graphicsQueue = device.getQueue(queueFamily.graphicsFamily.value(), 0);
	presentQueue = device.getQueue(queueFamily.presentFamily.value(), 0);
	LLOG_INFO << "Available Extensions:";

	for (const auto& extension : vk::enumerateInstanceExtensionProperties())
		LLOG_INFO << "\t" << extension.extensionName;

	std::tie(swapchain, swapchainImageFormat,
			 swapchainExtent) = init_createSwapchain(
									window, physicalDevice, device, surface,
									queueFamily);
	swapchainImages = device.getSwapchainImagesKHR(swapchain);
	swapchainImageViews = init_createImageViews(device, swapchainImages,
						  swapchainImageFormat);
	pipelineLayout = init_createPipelineLayout(device);
	renderPass = init_createRenderPass(device, swapchainImageFormat);
	std::tie(pipeline, shaderModules) = init_createGraphicsPipeline(device,
										pipelineLayout, renderPass);
	swapchainFramebuffers = init_createFramebuffer(device, renderPass,
							swapchainExtent, swapchainImageViews);
	commandPool = init_createCommandPool(device, queueFamily);
	commandBuffers = init_createCommandBuffers(device, commandPool,
					 MAX_FRAMES_IN_FLIGHT);
	std::tie(isImageAvailable, isRenderingFinished,
			 isRenderingInFlight) = init_createSyncObjects(device, MAX_FRAMES_IN_FLIGHT);
}

GraphicsDeviceInterface::~GraphicsDeviceInterface() {
	device.waitIdle();

	for (const vk::Semaphore& semaphore : isImageAvailable)
		device.destroySemaphore(semaphore);

	for (const vk::Semaphore& semaphore : isRenderingFinished)
		device.destroySemaphore(semaphore);

	for (const vk::Fence& fence : isRenderingInFlight)
		device.destroyFence(fence);

	device.destroyCommandPool(commandPool);

	for (const vk::Framebuffer& framebuffer : swapchainFramebuffers)
		device.destroyFramebuffer(framebuffer);

	device.destroyPipelineLayout(pipelineLayout);
	device.destroyRenderPass(renderPass);

	for (const vk::ShaderModule& shaderModule : shaderModules)
		device.destroyShaderModule(shaderModule);

	device.destroyPipeline(pipeline);

	for (const vk::ImageView& imageView : swapchainImageViews)
		device.destroyImageView(imageView);

	device.destroySwapchainKHR(std::move(swapchain));
	device.destroy();

	if (Validation::shouldEnableValidationLayers)
		instance.destroyDebugUtilsMessengerEXT(debugUtilsMessenger);

	instance.destroySurfaceKHR(surface);
	instance.destroy();
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void GraphicsDeviceInterface::recordCommandBuffer(
	vk::CommandBuffer buffer,
	uint32_t imageIndex
) {
	vk::CommandBufferBeginInfo beginInfo({}, nullptr);
	buffer.begin(beginInfo);
	vk::ClearValue clearColor(vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f));
	vk::RenderPassBeginInfo renderPassInfo(
		renderPass,
		swapchainFramebuffers[imageIndex],
		vk::Rect2D(vk::Offset2D {0, 0}, swapchainExtent),
		1,
		&clearColor
	);
	buffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
	buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(swapchainExtent.width),
						  static_cast<float>(swapchainExtent.height), 0.0f, 1.0f);
	buffer.setViewport(0, 1, &viewport);
	vk::Rect2D scissor(vk::Offset2D(0.0f, 0.0f), swapchainExtent);
	buffer.setScissor(0, 1, &scissor);
	buffer.draw(3, 1, 0, 0);
	buffer.endRenderPass();
	buffer.end();
}

bool GraphicsDeviceInterface::drawFrame() {
	const uint64_t no_time_limit = std::numeric_limits<uint64_t>::max();
	vk::Result result = device.waitForFences(1, &isRenderingInFlight[currentFrame],
						vk::True,
						no_time_limit);
	ASSERT(result == vk::Result::eSuccess,
		   "Can't wait for previous frame rendering");
	result = device.resetFences(1, &isRenderingInFlight[currentFrame]);
	ASSERT(result == vk::Result::eSuccess,
		   "Can't reset fence after render");

	if (result != vk::Result::eSuccess)
		return false;

	const vk::ResultValue<uint32_t> imageIndex = device.acquireNextImageKHR(
			swapchain, no_time_limit, isImageAvailable[currentFrame], nullptr);
	ASSERT(imageIndex.result == vk::Result::eSuccess,
		   "Can't acquire next image in swapchain");
	recordCommandBuffer(commandBuffers[currentFrame], imageIndex.value);
	const vk::PipelineStageFlags waitStage =
		vk::PipelineStageFlagBits::eColorAttachmentOutput;
	const vk::SubmitInfo submitInfo(1, &isImageAvailable[currentFrame],
									&waitStage, 1, &commandBuffers[currentFrame], 1,
									&isRenderingFinished[currentFrame]);
	result = graphicsQueue.submit(1, &submitInfo,
								  isRenderingInFlight[currentFrame]);
	ASSERT(result == vk::Result::eSuccess,
		   "Can't submit graphics queue");

	if (result != vk::Result::eSuccess)
		return false;

	vk::PresentInfoKHR presentInfo(1, &isRenderingFinished[currentFrame], 1,
								   &swapchain, &imageIndex.value, nullptr);
	result = presentQueue.presentKHR(presentInfo);
	ASSERT(result == vk::Result::eSuccess,
		   "Can't present image");

	if (result != vk::Result::eSuccess)
		return false;

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	return true;
}

#include "low_level_renderer/graphics_module.h"

#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_vulkan.h"
#include "core/logger/vulkan_ensures.h"
#include "imgui.h"

namespace graphics {
Module Module::create() {
	ResourceManager resources;
	GraphicsDeviceInterface device =
		GraphicsDeviceInterface::createGraphicsDevice(resources);
	GraphicsUserInterface ui = GraphicsUserInterface::create(device);
	return Module{
		.resources = resources, .device = std::move(device), .ui = ui
	};
}

void Module::destroy() {
	VULKAN_ENSURE_SUCCESS_EXPR(
		device.device.waitIdle(), "Can't wait for device idle:"
	);
	resources.meshes.destroyBy(device.device);
	graphics::destroy(materials, device.device);
	graphics::destroy(textures, device.device);
	resources.shaders.destroyBy(device.device);
	instances.destroyBy(device.device);
	ui.destroy(device);
	device.destroy();
}

void Module::beginFrame() {
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	ImGuiIO& io = ImGui::GetIO();
	(void)io;

	ImGui::Begin("General debugging");
	ImGui::Text(
		"Application average %.3f ms/frame (%.1f FPS)",
		1000.0f / io.Framerate,
		io.Framerate
	);
	ImGui::End();
}

void Module::handleEvent(const SDL_Event& event) {
	device.handleEvent(event);
	ui.handleEvent(event, device);
}

bool Module::drawFrame(
	const RenderSubmission& renderSubmission, GPUSceneData& sceneData
) {
	ASSERT(device.swapchain, "Attempt to draw frame without a swapchain");

	// Render ImGui. By this point all ImGui calls must be processed. If
	// creating UI for debugging this draw frame process, then we would want to
	// move this to another spot
	ImGui::Render();

	device.writeBuffer.batchWrite(device.device);

	GraphicsDeviceInterface::FrameData& currentFrame =
		device.frameDatas[device.currentFrame];

	const vk::Device device = this->device.device;

	const uint64_t no_time_limit = std::numeric_limits<uint64_t>::max();
	VULKAN_ENSURE_SUCCESS_EXPR(
		this->device.device.waitForFences(
			1, &currentFrame.isRenderingInFlight, vk::True, no_time_limit
		),
		"Can't wait for previous frame rendering:"
	);
	const vk::ResultValue<uint32_t> imageIndex = device.acquireNextImageKHR(
		this->device.swapchain->swapchain,
		no_time_limit,
		currentFrame.isImageAvailable,
		nullptr
	);

	switch (imageIndex.result) {
		case vk::Result::eErrorOutOfDateKHR:
			LLOG_INFO << "Out of date KHR";
			this->device.recreateSwapchain();
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

	currentFrame.sceneDataBuffer.update(sceneData);

	recordCommandBuffer(renderSubmission, commandBuffer, imageIndex.value);

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
		this->device.graphicsQueue.submit(
			1, &submitInfo, currentFrame.isRenderingInFlight
		),
		"Can't submit graphics queue:"
	);
	vk::PresentInfoKHR presentInfo(
		1,
		&currentFrame.isRenderingFinished,
		1,
		&this->device.swapchain->swapchain,
		&imageIndex.value,
		nullptr
	);

	switch (this->device.presentQueue.presentKHR(presentInfo)) {
		case vk::Result::eErrorOutOfDateKHR:
		case vk::Result::eSuboptimalKHR:	 this->device.recreateSwapchain();
		case vk::Result::eSuccess:			 break;
		default:
			LLOG_ERROR << "Error submitting to present queue: "
					   << vk::to_string(imageIndex.result);
			return false;
	}
	return true;
}

void Module::endFrame() {
	device.currentFrame = (device.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Module::recordCommandBuffer(
	const RenderSubmission& renderSubmission,
	vk::CommandBuffer buffer,
	uint32_t imageIndex
) {
	ASSERT(device.swapchain, "Attempt to record command buffer");
	vk::CommandBufferBeginInfo beginInfo({}, nullptr);
	VULKAN_ENSURE_SUCCESS_EXPR(
		buffer.begin(beginInfo), "Can't begin recording command buffer:"
	);

	vk::Rect2D screenExtent(vk::Offset2D{0, 0}, device.swapchain->extent);
	vk::Viewport viewport(
		0.0f,
		0.0f,
		static_cast<float>(device.swapchain->extent.width),
		static_cast<float>(device.swapchain->extent.height),
		0.0f,
		1.0f
	);
	buffer.setViewport(0, 1, &viewport);
	vk::Rect2D scissor(vk::Offset2D(0.0f, 0.0f), device.swapchain->extent);
	buffer.setScissor(0, 1, &scissor);

	{  // Main Renderpass
		const std::array<vk::ClearValue, 2> clearColors{
			vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f),
			vk::ClearColorValue(1.0f, 0.0f, 0.0f, 0.0f)
		};
		const vk::RenderPassBeginInfo renderPassInfo(
			device.renderPass,
			device.swapchain->framebuffers[imageIndex],
			screenExtent,
			static_cast<uint32_t>(clearColors.size()),
			clearColors.data()
		);
		buffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
		buffer.bindPipeline(
			vk::PipelineBindPoint::eGraphics,
			device.pipeline.regularPipeline.pipeline
		);

		buffer.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics,
			device.pipeline.regularPipeline.layout,
			static_cast<int>(PipelineDescriptorSetBindingPoint::eGlobal),
			1,
			&device.frameDatas[device.currentFrame].globalDescriptor,
			0,
			nullptr
		);

		renderSubmission.recordNonInstanced(
			buffer,
			device.pipeline.regularPipeline.layout,
			materials,
			resources.meshes,
			device.currentFrame
		);

		buffer.bindPipeline(
			vk::PipelineBindPoint::eGraphics,
			device.pipeline.instanceRenderingPipeline.pipeline
		);

		buffer.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics,
			device.pipeline.instanceRenderingPipeline.layout,
			static_cast<int>(PipelineDescriptorSetBindingPoint::eGlobal),
			1,
			&device.frameDatas[device.currentFrame].globalDescriptor,
			0,
			nullptr
		);

		renderSubmission.recordInstanced(
			buffer,
			device.pipeline.instanceRenderingPipeline.layout,
			instances,
			materials,
			resources.meshes,
			device.currentFrame
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

TextureID Module::loadTexture(const char* filePath) {
	return pushTextureFromFile(
		textures,
		filePath,
		device.device,
		device.physicalDevice,
		device.commandPool,
		device.graphicsQueue
	);
}

MeshID Module::loadMesh(const char* filePath) {
	return resources.meshes.load(
		device.device,
		device.physicalDevice,
		device.commandPool,
		device.graphicsQueue,
		filePath
	);
}

MaterialInstanceID Module::loadMaterial(
	TextureID albedo,
	TextureID normal,
	TextureID displacementMap,
	MaterialProperties properties,
	SamplerType samplerType
) {
	vk::Sampler sampler = samplerType == SamplerType::eLinear
							  ? device.samplers.linear
							  : device.samplers.point;
	return ::graphics::create(
		materials,
		textures,
		albedo,
		normal,
        displacementMap,
		properties,
		device.device,
		device.physicalDevice,
		device.pipeline.materialDescriptor.setLayout,
		sampler,
		device.pipeline.materialDescriptor.allocator,
		device.writeBuffer
	);
}

RenderInstanceID Module::registerInstance(uint16_t numberOfEntries) {
	return instances.create(
		device.device,
		device.physicalDevice,
		device.pipeline.instanceRenderingDescriptor.setLayout,
		device.pipeline.instanceRenderingDescriptor.allocator,
		device.writeBuffer,
		numberOfEntries
	);
}
}  // namespace graphics

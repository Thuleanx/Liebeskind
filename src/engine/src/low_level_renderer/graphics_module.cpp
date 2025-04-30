#include "low_level_renderer/graphics_module.h"

#include <glslang/Public/ShaderLang.h>

#include "game_specific/cameras/module.h"
#include "low_level_renderer/render_submission.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"
#include "imgui_internal.h"
#pragma GCC diagnostic pop

#include "core/logger/vulkan_ensures.h"

namespace graphics {
std::optional<Module> module = std::nullopt;

Module Module::create() {
	glslang::InitializeProcess();
	LLOG_INFO << "glslang initialized with version: "
			  << glslang::GetGlslVersionString();

	ShaderStorage shaders = ShaderStorage::create();
	GraphicsDeviceInterface device =
		GraphicsDeviceInterface::createGraphicsDevice(shaders);
	GraphicsUserInterface ui = GraphicsUserInterface::create(device);

	LLOG_INFO << "Graphics Module Initialized";
	return Module{
		.device = std::move(device),
		.ui = ui,
		.instances = {},
		.shaders = shaders,
		.textures = {},
		.materials = MaterialStorage::create(),
		.meshes = MeshStorage::create(),
		.mainWindowExtent = {},
	};
}

void Module::destroy() {
	VULKAN_ENSURE_SUCCESS_EXPR(
		device.device.waitIdle(), "Can't wait for device idle:"
	);
	graphics::destroy(meshes, device.device);
	graphics::destroy(materials, device.device);
	graphics::destroy(textures, device.device);
	graphics::destroy(shaders, device.device);
	instances.destroyBy(device.device);
	ui.destroy(device);
	device.destroy();
	LLOG_INFO << "Graphics Module Destroyed";

	glslang::FinalizeProcess();
}

void Module::beginFrame() {
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	ImGuiID dockSpaceID = ImGui::DockSpaceOverViewport(
		0,
		ImGui::GetMainViewport(),
		ImGuiDockNodeFlags_PassthruCentralNode |
			ImGuiDockNodeFlags_NoDockingOverCentralNode
	);
	ImGuiViewport* mainViewport = ImGui::GetMainViewport();
	ImGuiDockNode* centralNode = ImGui::DockBuilderGetCentralNode(dockSpaceID);
	mainWindowExtent = vk::Rect2D{
		vk::Offset2D{
			static_cast<int32_t>(centralNode->Pos.x - mainViewport->Pos.x),
			static_cast<int32_t>(centralNode->Pos.y - mainViewport->Pos.y)
		},
		{static_cast<uint32_t>(centralNode->Size.x),
		 static_cast<uint32_t>(centralNode->Size.y)}
	};
	cameras::module->mainCamera.setAspectRatio(
		static_cast<float>(mainWindowExtent.extent.width) /
		mainWindowExtent.extent.height
	);
}

void Module::handleEvent(const SDL_Event& event) {
	device.handleEvent(event);
	ui.handleEvent(event, device);
}

bool Module::drawFrame(
	RenderSubmission& renderSubmission, GPUSceneData& sceneData
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

	ImGuiIO& io = ImGui::GetIO();
	// Update and Render additional Platform Windows
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

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
	RenderSubmission& renderSubmission,
	vk::CommandBuffer buffer,
	uint32_t imageIndex
) {
	ASSERT(device.swapchain, "Attempt to record command buffer");
	vk::CommandBufferBeginInfo beginInfo({}, nullptr);
	VULKAN_ENSURE_SUCCESS_EXPR(
		buffer.begin(beginInfo), "Can't begin recording command buffer:"
	);

	const vk::Rect2D screenExtent = {
		vk::Offset2D{},
		vk::Extent2D{
			device.swapchain->extent.width,
			device.swapchain->extent.height,
		}
	};
	const vk::Viewport viewport(
		mainWindowExtent.offset.x,
		mainWindowExtent.offset.y,
		mainWindowExtent.extent.width,
		mainWindowExtent.extent.height,
		0.0f,
		1.0f
	);
	buffer.setViewport(0, 1, &viewport);
	const vk::Rect2D scissor = mainWindowExtent;
	buffer.setScissor(0, 1, &scissor);

	{  // Main Renderpass
		const std::array<vk::ClearValue, 3> clearColors{
			vk::ClearColorValue(
				0.0f, 0.0f, 0.0f, 1.0f
			),	// for multisample color buffer
			vk::ClearColorValue(1.0f, 0.0f, 0.0f, 0.0f),  // for depth buffer
			vk::ClearColorValue(
				0.0f, 0.0f, 0.0f, 1.0f
			),	// for regular color buffer
		};
		const vk::RenderPassBeginInfo renderPassInfo(
			device.renderPasses.mainPass,
			device.swapchain->mainFramebuffers[imageIndex],
			mainWindowExtent,
			static_cast<uint32_t>(clearColors.size()),
			clearColors.data()
		);
		buffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
		prepForRecording(renderSubmission, instances, device.currentFrame);

		buffer.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics,
			device.pipeline.regularPipelineLayout,
			static_cast<int>(MainPipelineDescriptorSetBindingPoint::eGlobal),
			1,
			&device.frameDatas[device.currentFrame].globalDescriptor,
			0,
			nullptr
		);

		recordRegularDrawCalls(
			renderSubmission,
			buffer,
			device.pipeline.regularPipelineLayout,
			device.pipeline,
			materials,
			meshes
		);

		buffer.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics,
			device.pipeline.instanceRenderingPipelineLayout,
			static_cast<int>(MainPipelineDescriptorSetBindingPoint::eGlobal),
			1,
			&device.frameDatas[device.currentFrame].globalDescriptor,
			0,
			nullptr
		);

		recordInstancedDrawCalls(
			renderSubmission,
			buffer,
			device.pipeline.instanceRenderingPipelineLayout,
            instances,
            device.pipeline,
            materials,
            meshes,
            device.currentFrame
		);

		buffer.endRenderPass();
	}

	{  // Post processing RenderPass
		const std::array<vk::ClearValue, 1> clearColors = {
			vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f)
		};

		const vk::RenderPassBeginInfo renderPassInfo(
			device.renderPasses.postProcessingPass,
			device.swapchain->postProcessingFramebuffers[imageIndex],
			mainWindowExtent,
			clearColors.size(),
			clearColors.data()
		);
		buffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

		buffer.bindPipeline(
			vk::PipelineBindPoint::eGraphics,
			device.pipeline.postProcessingPipeline.pipeline
		);

		buffer.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics,
			device.pipeline.postProcessingPipeline.layout,
			static_cast<int>(PostProcessingDescriptorSetBindingPoint::eGlobal),
			1,
			&device.frameDatas[imageIndex].postProcessingDescriptor,
			0,
			nullptr
		);

		buffer.draw(6, 1, 0, 0);

		buffer.endRenderPass();
	}

	const vk::Viewport screenViewport(
		0, 0, screenExtent.extent.width, screenExtent.extent.height, 0.0f, 1.0f
	);
	buffer.setViewport(0, 1, &screenViewport);
	buffer.setScissor(0, 1, &screenExtent);

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

TextureID Module::loadTexture(
	std::string_view filePath, vk::Format imageFormat
) {
	return pushTextureFromFile(
		textures,
		filePath,
		device.device,
		device.physicalDevice,
		device.commandPool,
		device.graphicsQueue,
		imageFormat
	);
}

MeshID Module::loadMesh(
	const std::vector<graphics::Vertex>& vertices,
	const std::vector<graphics::IndexType>& indices
) {
	return load(
		meshes,
		vertices,
		indices,
		device.device,
		device.physicalDevice,
		device.commandPool,
		device.graphicsQueue
	);
}

MeshID Module::loadMesh(std::string_view filePath) {
	return load(
		meshes,
		device.device,
		device.physicalDevice,
		device.commandPool,
		device.graphicsQueue,
		filePath
	);
}

MaterialInstanceID Module::loadMaterial(const MaterialCreateInfo& createInfo) {
	vk::Sampler sampler = createInfo.sampler == SamplerType::eLinear
							  ? device.samplers.linear
							  : device.samplers.point;
	return ::graphics::create(
		materials,
		textures,
		createInfo,
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

void Module::createPipelineVariant(
    const PipelineSpecializationConstants& specializationConstants
) {
	const bool isPipelineMissing =
		!device.pipeline.regularPipelineVariants.contains(specializationConstants);
	if (isPipelineMissing) {
		createNewVariant(
			device.pipeline,
			specializationConstants,
			device.device,
			device.renderPasses.mainPass,
			getModule(shaders, device.mainShaders.vertex),
			getModule(shaders, device.mainShaders.vertexInstanced),
			getModule(shaders, device.mainShaders.fragment)
		);
	}
}

void Module::updateMaterial(
	MaterialInstanceID material, const MaterialProperties& properties
) const {
	update(materials, properties, material);
}

}  // namespace graphics

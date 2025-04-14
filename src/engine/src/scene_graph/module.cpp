#include "scene_graph/module.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "core/logger/assert.h"
#include "game_specific/cameras/module.h"
#include "game_specific/cameras/perspective_camera.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include "imgui.h"
#pragma GCC diagnostic pop

namespace scene_graph {
std::optional<Module> module;

Module Module::create() {
	graphics::GPUSceneData sceneData{
		.view = glm::mat4(1),
		.inverseView = glm::mat4(1.0),
		.projection = glm::mat4(1),
		.viewProjection = {},
		.ambientColor = glm::vec3(0.05),
		.mainLightDirection = glm::normalize(glm::vec3(-1.0, 0.0, 0.0)),
		.mainLightColor = glm::vec3(1, 1, 1),
	};
	return Module{
		.sceneData = sceneData,
		.renderSubmission = graphics::RenderSubmission::create(),
		.renderObjects = {},
		.instancedRenderObjects = {},
		.instancedRenderData = {}
	};
}

void Module::destroy() {}

void Module::addInstancedObjects(
	std::span<const graphics::InstancedRenderObject> instancedRenderObjects
) {
	this->instancedRenderObjects.insert(
		this->instancedRenderObjects.end(),
		instancedRenderObjects.begin(),
		instancedRenderObjects.end()
	);
	instancedRenderData.resize(this->instancedRenderObjects.size());
}

void Module::updateInstance(
	std::span<const size_t> indices,
	std::vector<std::span<const graphics::InstanceData>> data
) {
	ASSERT(
		indices.size() == data.size(),
		"Indices size (" << indices.size() << ") and data size (" << data.size()
						 << ") mismatched"
	);
	for (size_t i = 0; i < indices.size(); i++) {
		ASSERT(
			indices[i] < instancedRenderData.size(),
			"Index " << i << " is out of the range [0, "
					 << instancedRenderData.size() << ")"
		);
		std::vector<graphics::InstanceData> copiedData;
		copiedData.insert(copiedData.end(), data[i].begin(), data[i].end());
		instancedRenderData[indices[i]] = copiedData;
	}
}

void Module::addObjects(std::span<const graphics::RenderObject> renderObjects) {
	this->renderObjects.insert(
		this->renderObjects.end(), renderObjects.begin(), renderObjects.end()
	);
}

void Module::updateObjects(std::vector<std::tuple<int, glm::mat4>> updates) {
	for (const auto& [index, transform] : updates)
		this->renderObjects[index].transform = transform;
}

bool Module::drawFrame(graphics::Module& graphics) {
	cameras::PerspectiveCamera& mainCamera = cameras::module->mainCamera;

	ImGui::Begin("Graphics");
	ImGui::ColorEdit3("Ambient Color", glm::value_ptr(sceneData.ambientColor));
	ImGui::ColorEdit3("Light Color", glm::value_ptr(sceneData.mainLightColor));
	ImGui::SliderFloat3(
		"Light Direction",
		glm::value_ptr(sceneData.mainLightDirection),
		-1.0,
		1.0
	);
	ImGui::End();

	sceneData.view = mainCamera.view;
	sceneData.projection = mainCamera.projection;
	// accounts for difference between openGL and Vulkan clip space
	sceneData.projection[1][1] *= -1;
	sceneData.inverseView = glm::inverse(sceneData.view);
	sceneData.viewProjection = sceneData.projection * sceneData.view;

	renderSubmission.submit(renderObjects);
	renderSubmission.submit(instancedRenderObjects, instancedRenderData);

	bool isRenderSuccessful = graphics.drawFrame(renderSubmission, sceneData);
	renderSubmission.clear();
	return isRenderSuccessful;
}
}  // namespace scene_graph

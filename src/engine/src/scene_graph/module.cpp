#include "scene_graph/module.h"

#include "core/logger/assert.h"
#include "game_specific/cameras/module.h"
#include "game_specific/cameras/perspective_camera.h"

namespace scene_graph {
std::optional<Module> module;

Module Module::create() {
	return Module{.renderSubmission = graphics::RenderSubmission::create()};
}

void Module::destroy() {}

void Module::addInstancedObjects(
	std::span<graphics::InstancedRenderObject> instancedRenderObjects
) {
	this->instancedRenderObjects.insert(
		this->instancedRenderObjects.end(),
		instancedRenderObjects.begin(),
		instancedRenderObjects.end()
	);
	instancedRenderData.resize(this->instancedRenderObjects.size());
}

void Module::updateInstance(
	std::span<int> indices, std::vector<std::span<graphics::InstanceData>> data
) {
	ASSERT(
		indices.size() == data.size(),
		"Indices size (" << indices.size() << ") and data size (" << data.size()
						 << ") mismatched"
	);
	for (int i = 0; i < indices.size(); i++) {
		ASSERT(
			indices[i] >= 0 && indices[i] < instancedRenderData.size(),
			"Index " << i << " is out of the range [0, "
					 << instancedRenderData.size() << ")"
		);
		std::vector<graphics::InstanceData> copiedData;
		copiedData.insert(copiedData.end(), data[i].begin(), data[i].end());
		instancedRenderData[indices[i]] = copiedData;
	}
}

void Module::addObjects(std::span<graphics::RenderObject> renderObjects) {
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

	graphics::GPUSceneData sceneData{
		.view = mainCamera.view,
		.inverseView = glm::mat4(1.0),
		.projection = mainCamera.projection,
		.viewProjection = {},
		.ambientColor = glm::vec3(0.05),
		.mainLightDirection = glm::normalize(glm::vec3(-1.0, 0.0, 0.0)),
		.mainLightColor = glm::vec3(1, 1, 1),
	};
	// accounts for difference between openGL and Vulkan clip space
	sceneData.projection[1][1] *= -1;
	sceneData.inverseView = glm::inverse(sceneData.view);
	sceneData.viewProjection = sceneData.projection * sceneData.view;

	renderSubmission.submit(renderObjects);
	renderSubmission.submit(instancedRenderObjects, instancedRenderData);

	bool isRenderSuccessful = graphics.drawFrame(renderSubmission, sceneData);
	renderSubmission.clear();
	graphics.endFrame();
	return isRenderSuccessful;
}
}  // namespace scene_graph

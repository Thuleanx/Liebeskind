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
		.ambientColor = glm::vec3(1),
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

	ImGui::Begin("Render Objects");
	for (size_t i = 0; i < renderObjects.size(); i++) {
		const std::string label = "Object " + std::to_string(i);
		ImGui::TextColored(
			ImVec4(1, 0, 0, 1), "Object %ud", static_cast<uint32_t>(i)
		);

		graphics::PipelineSpecializationConstants& variant =
			renderObjects[i].variant;
		bool hasAlbedo =
			variant.samplerInclusion & graphics::SamplerInclusionBits::eAlbedo;
		bool hasNormal =
			variant.samplerInclusion & graphics::SamplerInclusionBits::eNormal;
		bool hasDisplacement = variant.samplerInclusion &
							   graphics::SamplerInclusionBits::eDisplacement;
		bool hasEmission = variant.samplerInclusion &
						   graphics::SamplerInclusionBits::eEmission;
		ImGui::Checkbox("Albedo: ", &hasAlbedo);
		ImGui::Checkbox("Normal: ", &hasNormal);
		ImGui::Checkbox("Displacement: ", &hasDisplacement);
		ImGui::Checkbox("Emission: ", &hasEmission);

		variant.samplerInclusion =
			hasAlbedo * graphics::SamplerInclusionBits::eAlbedo +
			hasNormal * graphics::SamplerInclusionBits::eNormal +
			hasDisplacement * graphics::SamplerInclusionBits::eDisplacement +
			hasEmission * graphics::SamplerInclusionBits::eEmission;

		const std::array<const char*, 4> labelParallaxMappingMethods = {
			"Basic", "Steep", "Parallax Occlusion", "Improv"
		};
		int parallaxMappingMode = static_cast<int>(variant.parallaxMappingMode);
		ImGui::Combo(
			"Parallax mapping method: ",
			&parallaxMappingMode,
			labelParallaxMappingMethods.data(),
			labelParallaxMappingMethods.size()
		);
		variant.parallaxMappingMode =
			static_cast<graphics::ParallaxMappingMode>(parallaxMappingMode);

		graphics.createPipelineVariant(variant);
	}
	ImGui::End();

	submit(renderSubmission, renderObjects);
	submit(renderSubmission, instancedRenderObjects, instancedRenderData);

	bool isRenderSuccessful = graphics.drawFrame(renderSubmission, sceneData);
	clear(renderSubmission);
	return isRenderSuccessful;
}
}  // namespace scene_graph

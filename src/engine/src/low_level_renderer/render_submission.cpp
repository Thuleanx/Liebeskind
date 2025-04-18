#include "low_level_renderer/render_submission.h"

#include <glm/gtx/string_cast.hpp>

#include "core/logger/assert.h"
#include "low_level_renderer/shader_data.h"

namespace graphics {
RenderSubmission RenderSubmission::create() { return RenderSubmission(); }

void RenderSubmission::submit(
	std::span<InstancedRenderObject> instances,
	std::span<std::vector<InstanceData>> data
) {
	ASSERT(
		instances.size() == data.size(),
		"Input number of instances "
			<< instances.size() << " is not the number of data" << data.size()
	);
	this->instances.insert(
		this->instances.end(), instances.begin(), instances.end()
	);
	this->instanceData.insert(
		this->instanceData.end(), data.begin(), data.end()
	);
}

void RenderSubmission::submit(std::span<const RenderObject> renderObjects) {
	for (const RenderObject& renderObject : renderObjects) {
		const MaterialInstanceID materialInstanceID =
			renderObject.materialInstance;
		if (!this->renderObjects.contains(materialInstanceID))
			this->renderObjects[materialInstanceID] =
				std::vector<SubmittedRenderObject>();
		this->renderObjects[materialInstanceID].push_back(SubmittedRenderObject{
			.transform = renderObject.transform,
			.mesh = renderObject.mesh,
		});
	}
}
void RenderSubmission::recordInstanced(
	vk::CommandBuffer buffer,
	vk::PipelineLayout pipelineLayout,
	const RenderInstanceManager& instanceManager,
	const MaterialStorage& materials,
	const MeshStorage& meshes,
	uint32_t currentFrame
) const {
	// update instance data
	for (size_t i = 0; i < instances.size(); i++) {
		instanceManager.update(
			instances[i].instance, currentFrame, instanceData[i]
		);
	}

	for (const InstancedRenderObject& instance : instances) {
		instanceManager.bind(
			buffer, pipelineLayout, instance.instance, currentFrame
		);
		bind(materials, instance.material, buffer, pipelineLayout);
		bind(meshes, buffer, instance.mesh);
		draw(meshes, buffer, instance.mesh, instance.count);
	}
}

void RenderSubmission::recordNonInstanced(
	vk::CommandBuffer buffer,
	vk::PipelineLayout pipelineLayout,
	const MaterialStorage& materials,
	const MeshStorage& meshes,
	[[maybe_unused]] uint32_t currentFrame
) const {
	for (const auto& [materialID, allRenderObjects] : renderObjects) {
		bind(materials, materialID, buffer, pipelineLayout);

		for (const SubmittedRenderObject& renderObject : allRenderObjects) {
			GPUPushConstants pushConstants = {.model = renderObject.transform};
			buffer.pushConstants(
				pipelineLayout,
				vk::ShaderStageFlagBits::eVertex,
				0,
				sizeof(GPUPushConstants),
				&pushConstants
			);
			bind(meshes, buffer, renderObject.mesh);
			draw(meshes, buffer, renderObject.mesh);
		}
	}
}

void RenderSubmission::clear() {
	instances.clear();
	instanceData.clear();
	renderObjects.clear();
}
}  // namespace graphics

#include "low_level_renderer/render_submission.h"

#include <algorithm>
#include <glm/gtx/string_cast.hpp>
#include <optional>

#include "core/logger/assert.h"
#include "low_level_renderer/shader_data.h"

namespace graphics {
RenderSubmission RenderSubmission::create() { return RenderSubmission(); }

void submit(
	RenderSubmission& renderSubmission,
	std::span<const InstancedRenderObject> instances,
	std::span<const std::vector<InstanceData>> data
) {
	ASSERT(
		instances.size() == data.size(),
		"Input number of instances "
			<< instances.size() << " is not the number of data" << data.size()
	);
	renderSubmission.instances.insert(
		renderSubmission.instances.end(), instances.begin(), instances.end()
	);
	renderSubmission.instanceData.insert(
		renderSubmission.instanceData.end(), data.begin(), data.end()
	);
}

void submit(
	RenderSubmission& renderSubmission,
	std::span<const RenderObject> renderObjects
) {
	renderSubmission.renderObjects.insert(
		end(renderSubmission.renderObjects),
		begin(renderObjects),
		end(renderObjects)
	);
}

void prepForRecording(
	RenderSubmission& renderSubmission,
	const RenderInstanceManager& instanceManager,
	uint32_t currentFrame
) {
	// update instance data
	for (size_t i = 0; i < renderSubmission.instances.size(); i++) {
		instanceManager.update(
			renderSubmission.instances[i].instance,
			currentFrame,
			renderSubmission.instanceData[i]
		);
	}

	std::sort(
		begin(renderSubmission.renderObjects),
		end(renderSubmission.renderObjects),
		[](const RenderObject& r0, const RenderObject& r1) -> bool {
			if (r0.variant == r1.variant) return r0.material < r1.material;
			return r0.variant < r1.variant;
		}
	);

	std::sort(
		renderSubmission.instances.begin(),
		renderSubmission.instances.end(),
		[](const InstancedRenderObject& r0,
		   const InstancedRenderObject& r1) -> bool {
			if (r0.variant == r1.variant) return r0.material < r1.material;
			return r0.variant < r1.variant;
		}
	);
}

void recordRegularDrawCalls(
	const RenderSubmission& renderSubmission,
	vk::CommandBuffer buffer,
	vk::PipelineLayout pipelineLayout,
	const MaterialPipeline& pipelines,
	const MaterialStorage& materials,
	const MeshStorage& meshes
) {
	std::optional<PipelineSpecializationConstants> boundVariant = std::nullopt;
	std::optional<MaterialInstanceID> boundMaterial = std::nullopt;

	for (const auto& [variant, transform, materialID, mesh] :
		 renderSubmission.renderObjects) {
		const bool shouldBindPipeline =
			!boundVariant.has_value() || boundVariant.value() != variant;
		if (shouldBindPipeline) {
			const vk::Pipeline pipeline =
				getRegularPipeline(pipelines, variant);
			buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
			boundVariant = variant;
		}

		const bool shouldBindMaterial =
			!boundMaterial.has_value() || boundMaterial.value() != materialID;
		if (shouldBindMaterial) {
			bind(materials, materialID, buffer, pipelineLayout);
			boundMaterial = materialID;
		}

		GPUPushConstants pushConstants = {.model = transform};
		buffer.pushConstants(
			pipelineLayout,
			vk::ShaderStageFlagBits::eVertex,
			0,
			sizeof(GPUPushConstants),
			&pushConstants
		);
		bind(meshes, buffer, mesh);
		draw(meshes, buffer, mesh);
	}
}

void recordInstancedDrawCalls(
	const RenderSubmission& renderSubmission,
	vk::CommandBuffer buffer,
	vk::PipelineLayout pipelineLayout,
	const RenderInstanceManager& instanceManager,
	const MaterialPipeline& pipelines,
	const MaterialStorage& materials,
	const MeshStorage& meshes,
	uint32_t currentFrame
) {
	std::optional<PipelineSpecializationConstants> boundVariant = std::nullopt;
	std::optional<MaterialInstanceID> boundMaterial = std::nullopt;
	for (const InstancedRenderObject& instance : renderSubmission.instances) {
		const bool shouldBindPipeline =
			!boundVariant.has_value() ||
			boundVariant.value() != instance.variant;
		if (shouldBindPipeline) {
			const vk::Pipeline pipeline =
				getInstanceRenderingPipeline(pipelines, instance.variant);
			buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
			boundVariant = instance.variant;
		}

		const bool shouldBindMaterial =
			!boundMaterial.has_value() ||
			boundMaterial.value() != instance.material;
		if (shouldBindMaterial) {
			bind(materials, instance.material, buffer, pipelineLayout);
			boundMaterial = instance.material;
		}

		instanceManager.bind(
			buffer, pipelineLayout, instance.instance, currentFrame
		);
		bind(meshes, buffer, instance.mesh);
		draw(meshes, buffer, instance.mesh, instance.count);
	}
}

void clear(RenderSubmission& renderSubmission) {
	renderSubmission.instances.clear();
	renderSubmission.instanceData.clear();
	renderSubmission.renderObjects.clear();
}
}  // namespace graphics

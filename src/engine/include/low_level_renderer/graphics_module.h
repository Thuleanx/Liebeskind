#pragma once

#include <optional>

#include "SDL3/SDL_events.h"
#include "low_level_renderer/graphics_device_interface.h"
#include "low_level_renderer/graphics_user_interface.h"
#include "low_level_renderer/instance_rendering.h"
#include "low_level_renderer/materials.h"
#include "low_level_renderer/render_submission.h"
#include "low_level_renderer/shaders.h"
#include "low_level_renderer/vertex_buffer.h"

namespace graphics {
struct Module {
	GraphicsDeviceInterface device;
	GraphicsUserInterface ui;
	RenderInstanceManager instances;
    ShaderStorage shaders;
	TextureStorage textures;
	MaterialStorage materials;
	MeshStorage meshes;
	vk::Rect2D mainWindowExtent;

   public:
	static Module create();
	void destroy();

	void beginFrame();
	void handleEvent(const SDL_Event& event);
	bool drawFrame(
		const RenderSubmission& renderSubmission, GPUSceneData& sceneData
	);
	void endFrame();

	[[nodiscard]] TextureID loadTexture(
		std::string_view filePath, vk::Format imageFormat
	);
	[[nodiscard]] MeshID loadMesh(
		const std::vector<graphics::Vertex>& vertices,
		const std::vector<graphics::IndexType>& indices
	);
	[[nodiscard]] MeshID loadMesh(std::string_view filePath);
	[[nodiscard]] MaterialInstanceID loadMaterial(
		TextureID albedo,
		TextureID normal,
		TextureID displacementMap,
		TextureID emissionMap,
		MaterialProperties properties,
		SamplerType samplerType
	);
	[[nodiscard]] RenderInstanceID registerInstance(uint16_t numberOfEntries);

	void updateMaterial(
		MaterialInstanceID material, const MaterialProperties& properties
	) const;

   private:
	void recordCommandBuffer(
		const RenderSubmission& renderSubmission,
		vk::CommandBuffer buffer,
		uint32_t image_index
	);
};

extern std::optional<Module> module;
}  // namespace graphics

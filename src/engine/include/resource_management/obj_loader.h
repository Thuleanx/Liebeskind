#pragma once

#include <vector>

#include "low_level_renderer/graphics_module.h"
#include "low_level_renderer/materials.h"
#include "low_level_renderer/meshes.h"

namespace resource_management {
struct Model {
	std::vector<graphics::PipelineSpecializationConstants> variants;
	std::vector<graphics::MeshID> meshes;
	std::vector<graphics::MaterialInstanceID> materials;
};

[[nodiscard]]
Model loadObj(
	graphics::Module& graphics,
	std::string_view modelPath,
	std::string_view mtlPath,
	std::string_view texturePath
);
};	// namespace resource_management

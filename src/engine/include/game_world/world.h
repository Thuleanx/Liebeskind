#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "low_level_renderer/materials.h"
#include "low_level_renderer/meshes.h"
#include "low_level_renderer/pipeline_template.h"
#include "scene_graph/module.h"

namespace game_world {

struct StaticObjects {
    std::vector<graphics::PipelineSpecializationConstants> variant;
	std::vector<glm::mat4> transform;
	std::vector<graphics::MaterialInstanceID> material;
	std::vector<graphics::MeshID> mesh;
};

struct World {
	StaticObjects statics;
};

void emplaceStatics(
	World& world,
    std::span<const graphics::PipelineSpecializationConstants> variants,
	std::span<const glm::mat4> transforms,
	std::span<const graphics::MaterialInstanceID> materials,
	std::span<const graphics::MeshID> meshes
);

void addToSceneGraph(const World& world, scene_graph::Module& sceneGraph);

}  // namespace game_world

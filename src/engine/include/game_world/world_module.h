#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "low_level_renderer/materials.h"
#include "resource_management/mesh_manager.h"

namespace game_world {

struct StaticObjects {
	std::vector<glm::mat4> transform;
    std::vector<graphics::MaterialInstanceID> material;
    std::vector<MeshID> mesh;
};

struct World {
	StaticObjects statics;
};
};	// namespace game_world

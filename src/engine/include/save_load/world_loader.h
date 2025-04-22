#pragma once

#include <glm/glm.hpp>

#include "game_world/world.h"
#include "low_level_renderer/graphics_module.h"
#include "serialized_world.h"

namespace save_load {

struct WorldLoader {
   public:
	virtual bool isValid(const SerializedWorld& serializedWorld) const;
	virtual void load(
		graphics::Module& graphics,
		game_world::World& world,
		const SerializedWorld& serializedWorld
	) const;
};

};	// namespace save_load

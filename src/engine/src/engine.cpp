#include "engine.h"

#include "game_specific/cameras/module.h"
#include "low_level_renderer/graphics_module.h"
#include "scene_graph/module.h"

namespace engine {
void init() {
	graphics::module = graphics::Module::create();
	cameras::module = cameras::Module::create();
	scene_graph::module = scene_graph::Module::create();
}

void destroy() {
	if (cameras::module.has_value()) {
		cameras::module->destroy();
		cameras::module = std::nullopt;
	}
	if (graphics::module.has_value()) {
		graphics::module->destroy();
		graphics::module = std::nullopt;
	}
	if (scene_graph::module.has_value()) {
		scene_graph::module->destroy();
		scene_graph::module = std::nullopt;
	}
}

};	// namespace engine

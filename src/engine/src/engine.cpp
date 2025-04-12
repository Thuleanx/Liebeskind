#include "engine.h"

#include "low_level_renderer/graphics_module.h"
#include "game_specific/cameras/module.h"

namespace engine {
void init() {
    graphics::module = graphics::Module::create();
    cameras::module = cameras::Module::create();
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
}

};	// namespace engine

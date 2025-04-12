#pragma once

#include <optional>

#include "game_specific/cameras/perspective_camera.h"

namespace cameras {
struct Module {
	PerspectiveCamera mainCamera;

   public:
	static Module create();
	void destroy();

	void handleScreenResize(int width, int height);
};

extern std::optional<Module> module;

}  // namespace cameras

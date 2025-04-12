#include "game_specific/cameras/module.h"

#include <glm/ext/matrix_transform.hpp>

namespace cameras {
std::optional<Module> module;

Module Module::create() {
	PerspectiveCamera camera = PerspectiveCamera::create(
		glm::lookAt(
			glm::vec3(10.0f, 0.0f, 3.0f),
			glm::vec3(0.0f, 0.0f, 3.f),
			glm::vec3(0.0f, 0.0f, 1.0f)
		),
		glm::radians(45.0f),
		16.0 / 9.0,
		0.1f,
		145.0f
	);
	return {.mainCamera = camera};
}

void Module::destroy() {}

void Module::handleScreenResize(int width, int height) {
	mainCamera.setAspectRatio(static_cast<float>(width) / height);
}
}  // namespace cameras

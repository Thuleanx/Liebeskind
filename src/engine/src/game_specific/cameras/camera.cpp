#include "game_specific/cameras/camera.h"

#include "core/math/transform.h"

namespace cameras {
Camera Camera::create(glm::mat4 view, glm::mat4 projection) {
	return Camera{view, projection, glm::inverse(view)};
}

glm::vec3 Camera::getRight() const { return math::getForward(transform); }

glm::vec3 Camera::getUp() const { return -math::getRight(transform); }

glm::vec3 Camera::getForward() const { return -math::getUp(transform); }
}  // namespace cameras

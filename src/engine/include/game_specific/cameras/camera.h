#pragma once

#include <glm/glm.hpp>

namespace cameras {
class Camera {
   public:
	glm::mat4 view;
	glm::mat4 projection;
	glm::mat4 transform;

   public:
	static Camera create(glm::mat4 view, glm::mat4 projection);

	glm::vec3 getRight() const;
	glm::vec3 getUp() const;
	glm::vec3 getForward() const;
};
}  // namespace cameras

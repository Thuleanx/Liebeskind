#include "game_specific/cameras/camera.h"
#include "core/math/transform.h"

Camera Camera::create(glm::mat4 view, glm::mat4 projection) {
    return Camera {
        view,
        projection,
        glm::inverse(view)
    };
}

glm::vec3 Camera::getRight() const { return Math::getForward(transform); }

glm::vec3 Camera::getUp() const { return -Math::getRight(transform); }

glm::vec3 Camera::getForward() const { return -Math::getUp(transform); }

#include "game_specific/cameras/camera.h"

Camera::Camera(glm::mat4 view, glm::mat4 projection) :
    view(view), projection(projection) {}

glm::mat4 Camera::getView() const { return view; }
glm::mat4 Camera::getProjection() const { return projection; }

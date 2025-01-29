#pragma once

#include <glm/glm.hpp>

class Camera {
   public:
    glm::mat4 getView() const;
    glm::mat4 getProjection() const;

   protected:
    Camera(glm::mat4 view, glm::mat4 projection);

    glm::mat4 view;
    glm::mat4 projection;
};

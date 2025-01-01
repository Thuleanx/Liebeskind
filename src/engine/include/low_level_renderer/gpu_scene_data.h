#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct GPUSceneData {
    glm::mat4 view;
    glm::mat4 projection;
    // for optimization, 1 less multiply in vertex shader
    glm::mat4 viewProjection;
    // lighting
    glm::vec4 ambientColor;
    glm::vec4 mainLightDirection;
    glm::vec4 mainLightColor;
};

struct GPUPushConstants {
    glm::mat4 model;
};

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct GPUSceneData {
    glm::mat4 view;
    glm::mat4 inverseView;
    glm::mat4 projection;
    // for optimization, 1 less multiply in vertex shader
    glm::mat4 viewProjection;
    // lighting
    alignas(16) glm::vec3 ambientColor;
    alignas(16) glm::vec3 mainLightDirection;
    alignas(16) glm::vec3 mainLightColor;
};

struct GPUPushConstants {
    glm::mat4 model;
};


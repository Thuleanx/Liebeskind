#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace graphics {
struct GPUSceneData {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 inverseView;
    alignas(16) glm::mat4 projection;
    // for optimization, 1 less multiply in vertex shader
    alignas(16) glm::mat4 viewProjection;
    // lighting
    alignas(16) glm::vec3 ambientColor;
    alignas(16) glm::vec3 mainLightDirection;
    alignas(16) glm::vec3 mainLightColor;
};

struct GPUPushConstants {
    alignas(16) glm::mat4 model;
};
}  // namespace Graphics

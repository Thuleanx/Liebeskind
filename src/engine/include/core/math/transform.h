#pragma once

#include <glm/fwd.hpp>
#include <glm/glm.hpp>

namespace math {
inline glm::vec3 getRight(const glm::mat4& localToWorld) {
	return localToWorld[1];
}

inline glm::vec3 getRightNormalized(const glm::mat4& localToWorld) {
	return glm::normalize(getRight(localToWorld));
}

inline glm::vec3 getForward(const glm::mat4& localToWorld) {
	return localToWorld[0];
}

inline glm::vec3 getForwardNormalized(const glm::mat4& localToWorld) {
	return glm::normalize(getForward(localToWorld));
}

inline glm::vec3 getUp(const glm::mat4& localToWorld) {
	return localToWorld[2];
}

inline glm::vec3 getUpNormalized(const glm::mat4& localToWorld) {
	return glm::normalize(getUp(localToWorld));
}

struct Transform {
    glm::vec3 position;
    glm::vec3 eulers;
    glm::vec3 scale;
};

glm::mat4 toMat4(const Transform& transform);

}  // namespace Math

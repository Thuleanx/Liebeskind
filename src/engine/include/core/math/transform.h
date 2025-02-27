#pragma once

#include <glm/fwd.hpp>
#include <glm/glm.hpp>

namespace Math {
inline glm::vec3 getRight(const glm::mat4& localToWorld) {
    return -glm::vec3(
        localToWorld[1][0], localToWorld[1][1], localToWorld[1][2]
    );
}

inline glm::vec3 getRightNormalized(const glm::mat4& localToWorld) {
    return glm::normalize(getRight(localToWorld));
}

inline glm::vec3 getForward(const glm::mat4& localToWorld) {
    return glm::vec3(
        localToWorld[0][0], localToWorld[0][1], localToWorld[0][2]
    );
}

inline glm::vec3 getForwardNormalized(const glm::mat4& localToWorld) {
    return glm::normalize(getForward(localToWorld));
}

inline glm::vec3 getUp(const glm::mat4& localToWorld) {
    return glm::vec3(
        localToWorld[2][0], localToWorld[2][1], localToWorld[2][2]
    );
}

inline glm::vec3 getUpNormalized(const glm::mat4& localToWorld) {
    return glm::normalize(getUp(localToWorld));
}
}  // namespace Math

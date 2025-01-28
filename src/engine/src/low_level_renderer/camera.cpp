#include "low_level_renderer/camera.h"
#include <glm/gtc/matrix_transform.hpp>

#include "logger/assert.h"

Camera Camera::create(
    glm::mat4 transform,
    float fieldOfView,
    float aspectRatio,
    float nearPlaneDistance,
    float farPlaneDistance
) {
    return Camera(
        transform,
        glm::inverse(transform),
        glm::perspective(
            fieldOfView, aspectRatio, nearPlaneDistance, farPlaneDistance
        ),
        fieldOfView,
        aspectRatio,
        nearPlaneDistance,
        farPlaneDistance
    );
}

Camera::Camera(
    glm::mat4 transform,
    glm::mat4 view,
    glm::mat4 projection,
    float fieldOfView,
    float aspectRatio,
    float nearPlaneDistance,
    float farPlaneDistance
) :
    transform(transform),
    fieldOfView(fieldOfView),
    aspectRatio(aspectRatio),
    nearPlaneDistance(nearPlaneDistance),
    farPlaneDistance(farPlaneDistance),
    view(view),
    projection(projection) {
    ASSERT(
        nearPlaneDistance < farPlaneDistance,
        "Near plane distance " << nearPlaneDistance
                               << " needs to be less than far plane distance "
                               << farPlaneDistance
    );
    ASSERT(
        fieldOfView > 0 && fieldOfView < glm::radians(360.0f),
        "Field of view of " << fieldOfView << " is not in the range [0, 2pi)"
    );
    ASSERT(
        aspectRatio > 0,
        "Aspect ratio of "
            << aspectRatio
            << " is invalid. This needs to be a positive quantity"
    );
}

glm::mat4 Camera::getTransform() const { return transform; }
glm::mat4 Camera::getView() const { return view; }
glm::mat4 Camera::getProjection() const { return projection; }

#include "game_specific/cameras/perspective_camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include "core/logger/assert.h"

PerspectiveCamera PerspectiveCamera::create(
    glm::mat4 view,
    float fieldOfView,
    float aspectRatio,
    float nearPlaneDistance,
    float farPlaneDistance
) {
    return PerspectiveCamera(
        glm::inverse(view),
        view,
        glm::perspective(
            fieldOfView, aspectRatio, nearPlaneDistance, farPlaneDistance
        ),
        fieldOfView,
        aspectRatio,
        nearPlaneDistance,
        farPlaneDistance
    );
}

PerspectiveCamera::PerspectiveCamera(
    glm::mat4 transform,
    glm::mat4 view,
    glm::mat4 projection,
    float fieldOfView,
    float aspectRatio,
    float nearPlaneDistance,
    float farPlaneDistance
) :
    Camera(view, projection),
    transform(transform),
    fieldOfView(fieldOfView),
    aspectRatio(aspectRatio),
    nearPlaneDistance(nearPlaneDistance),
    farPlaneDistance(farPlaneDistance) {
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

void PerspectiveCamera::setTransform(glm::mat4 newTransform) {
    transform = newTransform;
    view = glm::inverse(transform);
}

void PerspectiveCamera::setView(glm::mat4 newView) {
    view = newView;
    transform = glm::inverse(view);
}

void PerspectiveCamera::setFieldOfView(float newFieldOfView) {
    fieldOfView = newFieldOfView;
    recomputeProjection();
}

void PerspectiveCamera::setAspectRatio(float newAspectRatio) {
    aspectRatio = newAspectRatio;
    recomputeProjection();
}

void PerspectiveCamera::setNearPlaneDistance(float newNearPlaneDistance) {
    nearPlaneDistance = newNearPlaneDistance;
    recomputeProjection();
}

void PerspectiveCamera::setFarPlaneDistance(float newFarPlane) {
    farPlaneDistance = newFarPlane;
    recomputeProjection();
}

void PerspectiveCamera::recomputeProjection() {
    projection = glm::perspective(
        fieldOfView, aspectRatio, nearPlaneDistance, farPlaneDistance
    );
}

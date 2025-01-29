#pragma once

#include <glm/glm.hpp>

#include "camera.h"

class PerspectiveCamera : public Camera {
   public:
    static PerspectiveCamera create(
        glm::mat4 view,
        float fieldOfView = glm::radians(45.0f),
        float aspectRatio = 16 / 9.0,
        float nearPlaneDistance = 0.1,
        float farPlaneDistance = 100.0f
    );

    glm::mat4 getTransform() const { return transform; }
    float getFieldOfView() const { return fieldOfView; }
    float getAspectRatio() const { return aspectRatio; }
    float getNearPlane() const { return nearPlaneDistance; }
    float getFarPlane() const { return farPlaneDistance; }

    void setTransform(glm::mat4 newTransform);
    void setView(glm::mat4 newView);
    void setFieldOfView(float newFieldOfView);
    void setAspectRatio(float newAspectRatio);
    void setNearPlaneDistance(float newNearPlaneDistance);
    void setFarPlaneDistance(float newFarPlane);

   private:
    void recomputeProjection();

   private:
    PerspectiveCamera(
        glm::mat4 transform,
        glm::mat4 view,
        glm::mat4 projection,
        float fieldOfView,
        float aspectRatio,
        float nearPlaneDistance,
        float farPlaneDistance
    );

    glm::mat4 transform;
    float fieldOfView;
    float aspectRatio;  // width over height
    float nearPlaneDistance;
    float farPlaneDistance;
};

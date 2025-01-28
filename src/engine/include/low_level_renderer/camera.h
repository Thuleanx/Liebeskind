#pragma once

#include <glm/glm.hpp>

struct CameraInfo {
    glm::mat4 view;
    glm::mat4 projection;
};

class Camera {
   public:
    static Camera create(
        glm::mat4 transform,
        float fieldOfView = glm::radians(45.0f),
        float aspectRatio = 16 / 9.0,
        float nearPlaneDistance = 0.1,
        float farPlaneDistance = 100.0f
    );

    glm::mat4 getTransform() const;
    glm::mat4 getView() const;
    glm::mat4 getProjection() const;
    float getFieldOfView() const;
    float getAspectRatio() const;
    float getNearPlane() const;
    float getFarPlane() const;

    void setTransform(glm::mat4 newTransform);
    void setView(glm::mat4 newView);
    void setFieldOfView(float newFieldOfView);
    void setAspectRatio(float newAspectRatio);
    void setNearPlaneDistance(float newNearPlaneDistance);
    void setFarPlaneDistance(float newFarPlane);

   private:
    Camera(
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
    glm::mat4 view;  // inverse of transform matrix
    glm::mat4 projection;
};

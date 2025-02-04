#include "scene_graph/scene_drawer.h"

#include <chrono>

SceneDrawer::SceneDrawer(PerspectiveCamera camera) : camera(camera) {}

SceneDrawer SceneDrawer::create() {
    PerspectiveCamera camera = PerspectiveCamera::create(
        glm::lookAt(
            glm::vec3(-3.0f, 0.f, 0.5f),
            glm::vec3(0.0f, 0.0f, 0.5f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        ),
        glm::radians(45.0f),
        16.0 / 9.0,
        0.1f,
        145.0f
    );
    return camera;
}

void SceneDrawer::handleResize(int width, int height) {
    handleResize((float)width / height);
}

void SceneDrawer::handleResize(float aspectRatio) {
    camera.setAspectRatio(aspectRatio);
}

void SceneDrawer::addObjects(std::span<RenderObject> renderObjects) {
    this->renderObjects.insert(
        this->renderObjects.end(),
        renderObjects.begin(),
        renderObjects.end()
    );
}

void SceneDrawer::updateObjects(std::vector<std::tuple<int, glm::mat4>> updates) {
    for (const auto& [index, transform] : updates)
        this->renderObjects[index].transform = transform;
}

bool SceneDrawer::drawFrame(GraphicsDeviceInterface& device) {
    GPUSceneData sceneData{
        .view = camera.getView(),
        .inverseView = glm::mat4(1.0),
        .projection = camera.getProjection(),
        .viewProjection = {},
        .ambientColor = glm::vec3(0.05),
        .mainLightDirection = glm::normalize(glm::vec3(0.0, 1.0, -1)),
        .mainLightColor = glm::vec3(1, 1, 1),
    };
    // accounts for difference between openGL and Vulkan clip space
    sceneData.projection[1][1] *= -1;
    sceneData.inverseView = glm::inverse(sceneData.view);
    sceneData.viewProjection = sceneData.projection * sceneData.view;

    for (size_t i = 0; i < renderObjects.size(); i++) {
        renderSubmission.submit(renderObjects[i]);
    }

    bool isRenderSuccessful = device.drawFrame(renderSubmission, sceneData);
    renderSubmission.clear();
    return isRenderSuccessful;
}

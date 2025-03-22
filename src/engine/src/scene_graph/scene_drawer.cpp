#include "scene_graph/scene_drawer.h"

#include "core/logger/assert.h"

SceneDrawer::SceneDrawer(PerspectiveCamera camera) :
    camera(camera), renderSubmission(graphics::RenderSubmission::create()) {}

SceneDrawer SceneDrawer::create() {
    PerspectiveCamera camera = PerspectiveCamera::create(
        glm::lookAt(
            glm::vec3(-3.0f, 0.0f, 10.0f),
            glm::vec3(0.0f, 0.0f, 0.5f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        ),
        glm::radians(45.0f),
        16.0 / 9.0,
        0.1f,
        145.0f
    );
    return SceneDrawer(camera);
}

void SceneDrawer::handleResize(int width, int height) {
    handleResize((float)width / height);
}

void SceneDrawer::handleResize(float aspectRatio) {
    camera.setAspectRatio(aspectRatio);
}

void SceneDrawer::addInstancedObjects(
    std::span<graphics::InstancedRenderObject> instancedRenderObjects
) {
    this->instancedRenderObjects.insert(
        this->instancedRenderObjects.end(),
        instancedRenderObjects.begin(),
        instancedRenderObjects.end()
    );
    instancedRenderData.resize(this->instancedRenderObjects.size());
}

void SceneDrawer::updateInstance(
    std::span<int> indices, std::vector<std::span<graphics::InstanceData>> data
) {
    ASSERT(
        indices.size() == data.size(),
        "Indices size (" << indices.size() << ") and data size (" << data.size()
                         << ") mismatched"
    );
    for (int i = 0; i < indices.size(); i++) {
        ASSERT(
            indices[i] >= 0 && indices[i] < instancedRenderData.size(),
            "Index " << i << " is out of the range [0, "
                     << instancedRenderData.size() << ")"
        );
        std::vector<graphics::InstanceData> copiedData;
        copiedData.insert(copiedData.end(), data[i].begin(), data[i].end());
        instancedRenderData[indices[i]] = copiedData;
    }
}

void SceneDrawer::addObjects(std::span<graphics::RenderObject> renderObjects) {
    this->renderObjects.insert(
        this->renderObjects.end(), renderObjects.begin(), renderObjects.end()
    );
}

void SceneDrawer::updateObjects(std::vector<std::tuple<int, glm::mat4>> updates
) {
    for (const auto& [index, transform] : updates)
        this->renderObjects[index].transform = transform;
}

bool SceneDrawer::drawFrame(graphics::GraphicsModule& graphics) {
    graphics::GPUSceneData sceneData{
        .view = camera.view,
        .inverseView = glm::mat4(1.0),
        .projection = camera.projection,
        .viewProjection = {},
        .ambientColor = glm::vec3(0.05),
        .mainLightDirection = glm::normalize(glm::vec3(0.0, 1.0, -1)),
        .mainLightColor = glm::vec3(1, 1, 1),
    };
    // accounts for difference between openGL and Vulkan clip space
    sceneData.projection[1][1] *= -1;
    sceneData.inverseView = glm::inverse(sceneData.view);
    sceneData.viewProjection = sceneData.projection * sceneData.view;

    renderSubmission.submit(renderObjects);
    renderSubmission.submit(instancedRenderObjects, instancedRenderData);

    bool isRenderSuccessful =
        graphics.drawAndEndFrame(renderSubmission, sceneData);
    renderSubmission.clear();
    return isRenderSuccessful;
}

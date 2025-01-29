#include "scene_graph/scene_drawer.h"

#include <chrono>

SceneDrawer::SceneDrawer(PerspectiveCamera camera) :
    camera(camera), device(GraphicsDeviceInterface::createGraphicsDevice()) {
    camera.setAspectRatio(device.getAspectRatio());
}

SceneDrawer SceneDrawer::create() {
    PerspectiveCamera camera = PerspectiveCamera::create(
        glm::lookAt(
            glm::vec3(10.0f, 10.0f, 10.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        ),
        glm::radians(45.0f),
        16.0 / 9.0,
        0.1f,
        45.0f
    );
    SceneDrawer sceneDrawer(camera);

    TextureID albedo =
        sceneDrawer.device.loadTexture("textures/swordAlbedo.jpg");
    MeshID meshID = sceneDrawer.device.loadMesh("models/sword.obj");
    MaterialInstanceID material = sceneDrawer.device.loadMaterial(
        albedo,
        MaterialProperties{
            .specular = glm::vec3(8),
            .diffuse = glm::vec3(.4),
            .ambient = glm::vec3(1),
            .shininess = 16.0f
        },
        MaterialPass::OPAQUE
    );

    sceneDrawer.renderObjects.push_back(RenderObject{
        .transform = glm::mat4(1.0f),
        .materialInstance = material,
        .mesh = meshID,
    });

    return sceneDrawer;
}

void SceneDrawer::handleEvent(const SDL_Event& sdlEvent) {
    device.handleEvent(sdlEvent);
    camera.setAspectRatio(device.getAspectRatio());
}

bool SceneDrawer::drawFrame() {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(
                     currentTime - startTime
    )
                     .count();

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
        renderObjects[i].transform = glm::rotate(
            glm::mat4(1.0f),
            time * glm::radians(90.0f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );
        renderSubmission.submit(renderObjects[i]);
    }

    bool isRenderSuccessful = device.drawFrame(renderSubmission, sceneData);
    renderSubmission.clear();
    return isRenderSuccessful;
}

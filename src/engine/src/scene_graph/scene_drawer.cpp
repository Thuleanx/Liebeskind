#include "scene_graph/scene_drawer.h"

#include <chrono>

SceneDrawer::SceneDrawer() :
    device(GraphicsDeviceInterface::createGraphicsDevice()) {}

SceneDrawer SceneDrawer::create() {
    SceneDrawer sceneDrawer;

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

    sceneDrawer.renderObjects.push_back(
        RenderObject{.mesh = meshID, .transform = glm::mat4(1.0f)}
    );
    sceneDrawer.materials.push_back(material);

    return sceneDrawer;
}

void SceneDrawer::handleEvent(const SDL_Event& sdlEvent) {
    device.handleEvent(sdlEvent);
}

bool SceneDrawer::drawFrame() {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(
                     currentTime - startTime
    )
                     .count();

    for (size_t i = 0; i < renderObjects.size(); i++) {
        renderObjects[i].transform = glm::rotate(
            glm::mat4(1.0f),
            time * glm::radians(90.0f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );
        device.submitDrawRenderObject(renderObjects[i], materials[i]);
    }
    return device.drawFrame();
}

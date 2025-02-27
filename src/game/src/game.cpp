#include "game.h"

#include <chrono>
#include <glm/gtx/string_cast.hpp>

#include "backends/imgui_impl_sdl3.h"
#include "core/logger/logger.h"
#include "input_management.h"
#include "low_level_renderer/graphics_module.h"
#include "scene_graph/scene_drawer.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

void Game::run() {
    Logging::initializeLogger();

    GraphicsModule graphics = GraphicsModule::create();
    SceneDrawer sceneDrawer = SceneDrawer::create();
    sceneDrawer.handleResize(graphics.device.swapchain->getAspectRatio());

    TextureID albedo = graphics.loadTexture("textures/ChibiPippa.png");
    MeshID meshID = graphics.loadMesh("models/ChibiPippa.obj");
    MaterialInstanceID material = graphics.loadMaterial(
        albedo,
        MaterialProperties{
            .specular = glm::vec3(1),
            .diffuse = glm::vec3(.4),
            .ambient = glm::vec3(1),
            .shininess = 1.0f
        },
        MaterialPass::OPAQUE
    );

    glm::mat4 modelTransform = glm::mat4(1);

    RenderObject sword{
        .transform = modelTransform,
        .materialInstance = material,
        .mesh = meshID,
    };

    sceneDrawer.addObjects({std::addressof(sword), 1});

    float movementX = 0;
    float movementY = 0;
    float speed = 10;

    InputManager inputManager;

    inputManager.subscribe(Input::Ranged::MovementX, [&movementX](float value) {
        movementX = value;
    });
    inputManager.subscribe(Input::Ranged::MovementY, [&movementY](float value) {
        movementY = value;
    });

    static auto startTime = std::chrono::high_resolution_clock::now();
    float lastTime = 0;

    bool shouldQuitGame = false;

    const glm::vec3 up = glm::vec3(0,0,1);
    glm::vec3 right = sceneDrawer.camera.getRight();
    right = glm::normalize(right - up * glm::dot(up, right));
    glm::vec3 forward = sceneDrawer.camera.getForward();
    forward = glm::normalize(forward - up * glm::dot(up, forward));

    while (!shouldQuitGame) {
        graphics.beginFrame();

        SDL_Event sdlEvent;

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(
                         currentTime - startTime
        )
                         .count();
        float deltaTime = time - lastTime;

        while (SDL_PollEvent(&sdlEvent)) {
            switch (sdlEvent.type) {
                case SDL_EVENT_QUIT: shouldQuitGame = true; break;
                case SDL_EVENT_WINDOW_RESIZED:
                    sceneDrawer.handleResize(
                        sdlEvent.window.data1, sdlEvent.window.data2
                    );
                    break;
            }
            graphics.handleEvent(sdlEvent);
            inputManager.handleEvent(sdlEvent);
        }

        glm::vec3 frameMovement =
            speed *
            (movementX * right +
             movementY * forward) *
            deltaTime;

        modelTransform = glm::translate(modelTransform, frameMovement);

        sceneDrawer.updateObjects({{0, modelTransform}});

        if (!sceneDrawer.drawFrame(graphics)) break;
        lastTime = time;
    }

    graphics.destroy();
}

#pragma GCC diagnostic pop

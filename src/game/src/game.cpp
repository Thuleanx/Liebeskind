#include "game.h"

#include <chrono>

#include "core/logger/logger.h"
#include "input_management.h"
#include "scene_graph/scene_drawer.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

void Game::run() {
    Logging::initializeLogger();

    GraphicsDeviceInterface device =
        GraphicsDeviceInterface::createGraphicsDevice();
    SceneDrawer sceneDrawer = SceneDrawer::create();
    sceneDrawer.handleResize(device.getAspectRatio());

    TextureID albedo = device.loadTexture("textures/ChibiPippa.png");
    MeshID meshID = device.loadMesh("models/ChibiPippa.obj");
    MaterialInstanceID material = device.loadMaterial(
        albedo,
        MaterialProperties{
            .specular = glm::vec3(1),
            .diffuse = glm::vec3(.4),
            .ambient = glm::vec3(1),
            .shininess = 1.0f
        },
        MaterialPass::OPAQUE
    );

    glm::mat4 swordTransform = glm::mat4(1);

    RenderObject sword{
        .transform = swordTransform,
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

    while (!shouldQuitGame) {
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
            device.handleEvent(sdlEvent);
            inputManager.handleEvent(sdlEvent);
        }

        glm::vec3 frameMovement =
            speed * glm::vec3(movementX, movementY, 0) * deltaTime;

        swordTransform = glm::translate(swordTransform, frameMovement);

        sceneDrawer.updateObjects({{0, swordTransform}});

        if (!sceneDrawer.drawFrame(device)) break;
        lastTime = time;
    }
}

#pragma GCC diagnostic pop

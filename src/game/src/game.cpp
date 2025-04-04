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

	graphics::Module graphics = graphics::Module::create();
	SceneDrawer sceneDrawer = SceneDrawer::create();
	sceneDrawer.handleResize(graphics.device.swapchain->getAspectRatio());

	graphics::TextureID albedo =
		graphics.loadTexture("textures/robot_albedo.jpg");
	graphics::TextureID normalMap =
		graphics.loadTexture("textures/robot_normal.jpg");
	graphics::TextureID displacementMap =
		graphics.loadTexture("textures/robot_height.jpeg");
	MeshID meshID = graphics.loadMesh("models/robot.obj");
	graphics::MaterialInstanceID material = graphics.loadMaterial(
		albedo,
		normalMap,
		displacementMap,
		graphics::MaterialProperties{
			.specular = glm::vec3(0),
			.diffuse = glm::vec3(1),
			.ambient = glm::vec3(0),
			.shininess = 1.0f
		},
		graphics::SamplerType::eLinear
	);
	graphics::RenderInstanceID instance = graphics.registerInstance(10);
	graphics.device.writeBuffer.batchWrite(graphics.device.device);

	glm::mat4 modelTransform = glm::translate(
		glm::rotate(
			glm::scale(glm::mat4(1), glm::vec3(0.3)),
			glm::radians(90.f),
			glm::vec3(1.0, 0.0, 0.0)
		),
		glm::vec3(0.0, 0.0, 0.5)
	);

	graphics::RenderObject sword{
		.transform = modelTransform,
		.materialInstance = material,
		.mesh = meshID,
	};

	graphics::InstancedRenderObject instanceRenderObject{
		.instance = instance, .material = material, .mesh = meshID, .count = 9
	};

	std::vector<int> instanceIndices = {0};
	std::array<graphics::InstanceData, 9> instancesTransforms;
	for (int dx = -1; dx <= 1; dx++)
		for (int dy = -1; dy <= 1; dy++)
			instancesTransforms[dx + 1 + (dy + 1) * 3] = graphics::InstanceData{
				.transform =
					glm::translate(glm::mat4(1), glm::vec3(dx, dy, 0) * 3.0f)
			};

	sceneDrawer.addObjects({std::addressof(sword), 1});
	/* sceneDrawer.addInstancedObjects({std::addressof(instanceRenderObject),
	 * 1}); */
	/* sceneDrawer.updateInstance(instanceIndices, {{instancesTransforms}}); */

	float movementX = 0;
	float movementY = 0;
	float rotationInput = 0;
	float speed = 1;

	InputManager inputManager;

	inputManager.subscribe(Input::Ranged::MovementX, [&movementX](float value) {
		movementX = value;
	});
	inputManager.subscribe(Input::Ranged::MovementY, [&movementY](float value) {
		movementY = value;
	});
	inputManager.subscribe(
		Input::Ranged::Rotate,
		[&rotationInput](float value) { rotationInput = value; }
	);

	static auto startTime = std::chrono::high_resolution_clock::now();
	float lastTime = 0;

	bool shouldQuitGame = false;

	const glm::vec3 up = glm::vec3(0, 0, 1);
	glm::vec3 right = sceneDrawer.camera.getRight();
	right = glm::normalize(right - up * glm::dot(up, right));
	glm::vec3 forward = sceneDrawer.camera.getForward();
	forward = glm::normalize(forward - up * glm::dot(up, forward));

	int p = 0;
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
			speed * (movementX * right + movementY * forward) * deltaTime;

		modelTransform = glm::translate(glm::mat4(1), frameMovement) *
						 modelTransform *
						 glm::rotate(
							 glm::mat4(1),
							 glm::radians(rotationInput * 45.f * deltaTime),
							 glm::vec3(0, 1, 0)
						 );

		sceneDrawer.updateObjects({{0, modelTransform}});

		if (!sceneDrawer.drawFrame(graphics)) break;
		lastTime = time;
	}

	graphics.destroy();
}

#pragma GCC diagnostic pop

#include "game.h"

#include <chrono>
#include <glm/gtx/string_cast.hpp>

#include "backends/imgui_impl_sdl3.h"
#include "cameras/module.h"
#include "core/logger/logger.h"
#include "engine.h"
#include "game_specific/cameras/module.h"
#include "input_management.h"
#include "low_level_renderer/graphics_module.h"
#include "scene_graph/module.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

void game::init() {
	Logging::initializeLogger();
	engine::init();

	input::manager = input::Manager::create();
	game_cameras::module.emplace(game_cameras::Module::create());
}

void game::run() {
	graphics::TextureID albedo = graphics::module->loadTexture(
		"textures/bricks_albedo.jpg", vk::Format::eR8G8B8A8Srgb
	);
	graphics::TextureID normalMap = graphics::module->loadTexture(
		"textures/bricks_normal.jpg", vk::Format::eR8G8B8A8Unorm
	);
	graphics::TextureID displacementMap = graphics::module->loadTexture(
		"textures/bricks_height.jpg", vk::Format::eR8G8B8A8Unorm
	);
	graphics::TextureID emissionMap = graphics::module->loadTexture(
		"textures/robot_emissive.jpeg", vk::Format::eR8G8B8A8Unorm
	);

	MeshID meshID = graphics::module->loadMesh("models/quad.obj");
	graphics::MaterialInstanceID material = graphics::module->loadMaterial(
		albedo,
		normalMap,
		displacementMap,
		emissionMap,
		graphics::MaterialProperties{
			.specular = glm::vec3(1, 1, 1),
			.diffuse = glm::vec3(0.4),
			.ambient = glm::vec3(0),
			.emission = glm::vec3(0),
			.shininess = 128.0f
		},
		graphics::SamplerType::eLinear
	);
	graphics::RenderInstanceID instance =
		graphics::module->registerInstance(10);
	graphics::module->device.writeBuffer.batchWrite(
		graphics::module->device.device
	);

	glm::mat4 modelTransform = glm::translate(
		glm::rotate(
			glm::scale(glm::mat4(1), glm::vec3(3)),
			glm::radians(90.f),
			glm::vec3(0.0, 1.0, 0.0)
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
				.transform = glm::translate(
					glm::rotate(
						glm::scale(glm::mat4(1), glm::vec3(1)),
						glm::radians(90.f),
						glm::vec3(0.0, 1.0, 0.0)
					),
					glm::vec3(dx, dy, 0) * 2.0f
				)
			};

	scene_graph::module->addObjects({std::addressof(sword), 1});
	scene_graph::module->addInstancedObjects(
		{std::addressof(instanceRenderObject), 1}
	);
	scene_graph::module->updateInstance(
		instanceIndices, {{instancesTransforms}}
	);

	float movementX = 0;
	float movementY = 0;
	float rotationInput = 0;
	float speed = 1;

	input::manager->subscribe(
		input::Ranged::Rotate,
		[&rotationInput](float value) { rotationInput = value; }
	);

	static auto startTime = std::chrono::high_resolution_clock::now();
	float lastTime = 0;

	bool shouldQuitGame = false;

	int p = 0;
	while (!shouldQuitGame) {
		graphics::module->beginFrame();

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
					cameras::module->handleScreenResize(
						sdlEvent.window.data1, sdlEvent.window.data2
					);
					break;
			}
			graphics::module->handleEvent(sdlEvent);
			input::manager->handleEvent(sdlEvent);
		}

        game_cameras::module->update(deltaTime);

		modelTransform =
			modelTransform * glm::rotate(
								 glm::mat4(1),
								 glm::radians(rotationInput * 45.f * deltaTime),
								 glm::vec3(0, 1, 0)
							 );

		scene_graph::module->updateObjects({{0, modelTransform}});

		if (!scene_graph::module->drawFrame(graphics::module.value())) break;
		lastTime = time;
	}
}

void game::destroy() {
	if (game_cameras::module.has_value()) {
		game_cameras::module->destroy();
		game_cameras::module = std::nullopt;
	}
	if (input::manager.has_value()) { input::manager = std::nullopt; }
	engine::destroy();
}

#pragma GCC diagnostic pop

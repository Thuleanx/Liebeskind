#include "game.h"

#include <chrono>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include "backends/imgui_impl_sdl3.h"
#pragma GCC diagnostic pop

#include "cameras/module.h"
#include "core/logger/logger.h"
#include "engine.h"
#include "game_specific/cameras/module.h"
#include "input_management.h"
#include "low_level_renderer/graphics_module.h"
#include "scene_graph/module.h"

void game::init() {
	Logging::initializeLogger();
	engine::init();

	input::manager = input::Manager::create();
	game_cameras::module.emplace(game_cameras::Module::create());
}

void game::run() {
	const graphics::TextureID albedo = graphics::module->loadTexture(
		"textures/bricks_albedo.jpg", vk::Format::eR8G8B8A8Srgb
	);
	const graphics::TextureID normalMap = graphics::module->loadTexture(
		"textures/bricks_normal.jpg", vk::Format::eR8G8B8A8Unorm
	);
	const graphics::TextureID displacementMap = graphics::module->loadTexture(
		"textures/bricks_height.jpg", vk::Format::eR8G8B8A8Unorm
	);
	const graphics::TextureID emissionMap = graphics::module->loadTexture(
		"textures/robot_emissive.jpeg", vk::Format::eR8G8B8A8Unorm
	);

	const MeshID meshID = graphics::module->loadMesh("models/quad.obj");
	graphics::MaterialProperties materialProperties =
		graphics::MaterialProperties{
			.specular = glm::vec3(1, 1, 1),
			.diffuse = glm::vec3(0.4),
			.ambient = glm::vec3(0),
			.emission = glm::vec3(0),
			.shininess = 128.0f
		};
	const graphics::MaterialInstanceID material =
		graphics::module->loadMaterial(
			albedo,
			normalMap,
			displacementMap,
			emissionMap,
			materialProperties,
			graphics::SamplerType::eLinear
		);
	const graphics::RenderInstanceID instance =
		graphics::module->registerInstance(10);
	/* graphics::module->device.writeBuffer.batchWrite( */
	/* 	graphics::module->device.device */
	/* ); */

	glm::mat4 modelTransform = glm::translate(
		glm::rotate(
			glm::scale(glm::mat4(1), glm::vec3(3)),
			glm::radians(90.f),
			glm::vec3(0.0, 1.0, 0.0)
		),
		glm::vec3(0.0, 0.0, 0.5)
	);

	const graphics::RenderObject sword{
		.transform = modelTransform,
		.materialInstance = material,
		.mesh = meshID,
	};

	const graphics::InstancedRenderObject instanceRenderObject{
		.instance = instance, .material = material, .mesh = meshID, .count = 9
	};
	const std::vector<size_t> instanceIndices = {0};
	const std::array<graphics::InstanceData, 9> instancesTransforms = [&]() {
		std::array<graphics::InstanceData, 9> transforms;
		for (int dx = -1; dx <= 1; dx++)
			for (int dy = -1; dy <= 1; dy++)
				transforms[dx + 1 + (dy + 1) * 3] = graphics::InstanceData{
					.transform = glm::translate(
						glm::rotate(
							glm::scale(glm::mat4(1), glm::vec3(1)),
							glm::radians(90.f),
							glm::vec3(0.0, 1.0, 0.0)
						),
						glm::vec3(dx, dy, 0) * 2.0f
					)
				};
		return transforms;
	}();

	scene_graph::module->addObjects({std::addressof(sword), 1});
	scene_graph::module->addInstancedObjects(
		{std::addressof(instanceRenderObject), 1}
	);
	scene_graph::module->updateInstance(
		instanceIndices, {{instancesTransforms}}
	);

	float rotationInput = 0;
	input::manager->subscribe(
		input::Ranged::Rotate,
		[&rotationInput](float value) { rotationInput = value; }
	);

	static auto startTime = std::chrono::high_resolution_clock::now();
	float lastTime = 0;

	bool shouldQuitGame = false;

	while (!shouldQuitGame) {
		graphics::module->beginFrame();

		const auto currentTime = std::chrono::high_resolution_clock::now();
		const float time =
			std::chrono::duration<float, std::chrono::seconds::period>(
				currentTime - startTime
			)
				.count();
		const float deltaTime = time - lastTime;

		SDL_Event sdlEvent;
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

		const ImGuiIO& io = ImGui::GetIO();

		ImGui::Begin("General debugging");
		ImGui::Text(
			"Application average %.3f ms/frame (%.1f FPS)",
			1000.0f / io.Framerate,
			io.Framerate
		);
		ImGui::End();

		ImGui::Begin("Material Properties");
		ImGui::ColorEdit3(
			"Specular", glm::value_ptr(materialProperties.specular)
		);
		ImGui::InputFloat("Shininess", &materialProperties.shininess);
		ImGui::ColorEdit3(
			"Diffuse", glm::value_ptr(materialProperties.diffuse)
		);
		ImGui::ColorEdit3(
			"Ambient", glm::value_ptr(materialProperties.ambient)
		);
		ImGui::ColorEdit3(
			"Emission", glm::value_ptr(materialProperties.emission)
		);
		ImGui::End();

		game_cameras::module->update(deltaTime);

		modelTransform =
			modelTransform * glm::rotate(
								 glm::mat4(1),
								 glm::radians(rotationInput * 45.f * deltaTime),
								 glm::vec3(0, 1, 0)
							 );

		graphics::module->updateMaterial(material, materialProperties);
		scene_graph::module->updateObjects({{0, modelTransform}});

		if (!scene_graph::module->drawFrame(graphics::module.value())) break;

		graphics::module->endFrame();
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

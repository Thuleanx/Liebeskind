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
#include "save_load/json_serializer.h"
#include "save_load/world_loader.h"
#include "scene_graph/module.h"

void game::init() {
	Logging::initializeLogger();
	engine::init();

	input::manager = input::Manager::create();
	game_cameras::module.emplace(game_cameras::Module::create());
}

void game::run() {
	const save_load::JsonSerializer serializer = {};
	const save_load::WorldLoader worldLoader = {};
	const save_load::SerializedWorld serializedWorld =
		serializer.loadWorld("scenes/test_scene.json");

	ASSERT(worldLoader.isValid(serializedWorld), "Loaded world is not valid");
	game_world::World world = {};
	worldLoader.load(graphics::module.value(), world, serializedWorld);

	game_world::addToSceneGraph(world, scene_graph::module.value());

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

		game_cameras::module->update(deltaTime);

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

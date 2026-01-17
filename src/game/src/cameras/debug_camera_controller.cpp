#include "cameras/debug_camera_controller.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include "imgui.h"

namespace {
constexpr float AVERAGE_SCREEN_WIDTH = 1024;
constexpr float AVERAGE_SCREEN_HEIGHT = 768;
}  // namespace

namespace game_cameras {
void update(
	DebugCameraController& controller,
	cameras::PerspectiveCamera& camera,
	float deltaTime
) {
    ImGui::Begin("Debug camera");

    const glm::vec3 currentPosition = 
        glm::vec3(camera.transform[3]);

    ImGui::Text(
        "Position (%.3f, %.3f, %.3f)",
        currentPosition.x,
        currentPosition.y,
        currentPosition.z
    );

	constexpr glm::vec3 UP = glm::vec3(0, 0, 1);
	const glm::vec3 cameraRight = camera.getRight();
	const glm::vec3 cameraForward = camera.getForward();

	glm::vec3 right =
		glm::normalize(cameraRight - glm::dot(cameraRight, UP) * UP);
	glm::vec3 forward =
		glm::normalize(cameraForward - glm::dot(cameraForward, UP) * UP);

	const glm::vec3 normalizedInput = [&]() {
		if (abs(controller.currentXInput) < 1e-4 &&
			abs(controller.currentYInput) < 1e-4)
			return glm::vec3(0);
		return glm::normalize(
			right * controller.currentXInput +
			forward * controller.currentYInput
		);
	}();

	const glm::vec3 frameMovement =
		controller.movementSpeed *
		(normalizedInput +
		 UP * static_cast<float>(controller.upInput - controller.downInput)) *
		deltaTime;

    ImGui::Text(
        "Frame movement (%.3f, %.3f, %.3f)",
        frameMovement.x,
        frameMovement.y,
        frameMovement.z
    );

	camera.setTransform(
		glm::translate(glm::mat4(1), frameMovement) * camera.transform
	);
    ImGui::End();
}

void onMouseDeltaXDebug(
	DebugCameraController& controller,
	cameras::PerspectiveCamera& camera,
	float value
) {
	glm::mat4 newCameraTransform = camera.transform;
	const glm::vec3 translation = glm::vec3(newCameraTransform[3]);
	newCameraTransform[3] = glm::vec4(0, 0, 0, 1);
	newCameraTransform =
		glm::rotate(
			glm::mat4(1),
			-value * glm::radians(controller.rotationSpeedDegrees) /
				AVERAGE_SCREEN_WIDTH,
			glm::vec3(0, 0, 1)
		) *
		newCameraTransform;
	newCameraTransform[3] = glm::vec4(translation, 1);

	camera.setTransform(newCameraTransform);
}

void onMouseDeltaYDebug(
	DebugCameraController& controller,
	cameras::PerspectiveCamera& camera,
	float value
) {
	glm::mat4 newCameraTransform = camera.transform;
	const glm::vec3 translation = glm::vec3(newCameraTransform[3]);
	newCameraTransform[3] = glm::vec4(0, 0, 0, 1);
	newCameraTransform =
		glm::rotate(
			glm::mat4(1),
			-value * glm::radians(controller.rotationSpeedDegrees) /
				AVERAGE_SCREEN_HEIGHT,
			camera.getRight()
		) *
		newCameraTransform;
	newCameraTransform[3] = glm::vec4(translation, 1);

	camera.setTransform(newCameraTransform);
}

}  // namespace game_cameras

#include "core/math/transform.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace math {
glm::mat4 toMat4(const Transform& transform) {
	const glm::mat4 scaled = glm::scale(glm::mat4(1), transform.scale);
	const glm::vec3 eulerAnglesInRadians = glm::radians(transform.eulers);
	const glm::mat4 rotated = glm::eulerAngleYXZ(
								  eulerAnglesInRadians.y,
								  eulerAnglesInRadians.x,
								  eulerAnglesInRadians.z
							  ) *
							  scaled;
    const glm::mat4 translated = glm::translate(rotated, transform.position);
    return translated;
}
}  // namespace math

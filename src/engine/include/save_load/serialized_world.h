#pragma once

#include <glm/glm.hpp>
#include <optional>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "core/math/transform.h"

namespace save_load {
using IDType = uint16_t;

struct SerializedTextures {
	std::vector<IDType> id;
	std::vector<vk::Format> format;
	std::vector<std::string> filePath;
};

struct SerializedMeshes {
	std::vector<IDType> id;
	std::vector<std::string> filePath;
};

struct SerializedMaterials {
	std::vector<IDType> id;
	std::vector<glm::vec3> specular;
	std::vector<glm::vec3> diffuse;
	std::vector<glm::vec3> ambient;
	std::vector<glm::vec3> emission;
	std::vector<float> shininess;
	std::vector<std::optional<IDType>> albedoMap;
	std::vector<std::optional<IDType>> normalMap;
	std::vector<std::optional<IDType>> displacementMap;
	std::vector<std::optional<IDType>> emissionMap;
	std::vector<std::string> sampler;
};

struct SerializedStatics {
	std::vector<math::Transform> transform;
	std::vector<IDType> material;
	std::vector<IDType> mesh;
};

struct SerializedWorld {
	SerializedTextures textures;
	SerializedMeshes meshes;
	SerializedMaterials materials;
	SerializedStatics statics;
};
}  // namespace save_load

#include "save_load/json_serializer.h"

#include <fstream>
#include <nlohmann/json.hpp>

#include "core/logger/assert.h"

namespace {
bool areKeysPresent(
	const nlohmann::json& data, const std::vector<std::string>& keys
) {
	for (std::string_view key : keys)
		if (!data.contains(key)) return false;
	return true;
}

inline auto safeGet(const nlohmann::json& data, std::string_view key) {
	ASSERT(
		data.contains(key), "Key " << key << " missing from " << data.dump()
	);
	return data[key];
}

vk::Format stringToFormat(std::string_view s) {
	if (s == "R8G8B8A8Srgb") return vk::Format::eR8G8B8A8Srgb;
	if (s == "R8G8B8A8Unorm") return vk::Format::eR8G8B8A8Unorm;

	LLOG_ERROR << "Format " << s << " conversation not specified";

	__builtin_unreachable();
}

glm::vec3 toVec3(const nlohmann::json& data) {
	ASSERT(
		data.type() == nlohmann::json::value_t::array,
		"Vec3 data must be an array of objects"
	);
	ASSERT(data.size() == 3, "Vec3 data must be an array of 3 objects");
	ASSERT(data[0].is_number(), "Vec3 data must be an array of numbers");
	return glm::vec3(data[0], data[1], data[2]);
}

math::Transform toTransform(const nlohmann::json& data) {
	ASSERT(
		data.type() == nlohmann::json::value_t::object,
		"Transform data must be an object consisting of keys [position, "
		"rotation, scale]"
	);

	const glm::vec3 position = toVec3(safeGet(data, "position"));
	const glm::vec3 rotation = toVec3(safeGet(data, "rotation"));
	const glm::vec3 scale = toVec3(safeGet(data, "scale"));

	return math::Transform{
		.position = position,
		.eulers = rotation,
		.scale = scale,
	};
}

save_load::SerializedTextures loadTextures(const nlohmann::json& data) {
	if (data.is_null()) return save_load::SerializedTextures{};

	ASSERT(
		data.type() == nlohmann::json::value_t::array,
		"Texture data must be an array of objects"
	);

	const size_t numTextures = data.size();
	std::vector<save_load::IDType> ids;
	std::vector<vk::Format> formats;
	std::vector<std::string> filePaths;
	ids.reserve(numTextures);
	formats.reserve(numTextures);
	filePaths.reserve(numTextures);

	for (const nlohmann::json& entry : data) {
		const save_load::IDType id = safeGet(entry, "id");
		const vk::Format format =
			stringToFormat(safeGet(entry, "format").template get<std::string>()
			);
		const std::string filePath = safeGet(entry, "file_path");

		ids.push_back(id);
		formats.push_back(format);
		filePaths.push_back(filePath);
	}

	return save_load::SerializedTextures{
		.id = ids,
		.format = formats,
		.filePath = filePaths,
	};
}

save_load::SerializedMeshes loadMeshes(const nlohmann::json& data) {
	if (data.is_null()) return save_load::SerializedMeshes{};

	ASSERT(
		data.type() == nlohmann::json::value_t::array,
		"Mesh data must be an array of objects"
	);

	const size_t numTextures = data.size();

	std::vector<save_load::IDType> ids;
	std::vector<std::string> filePaths;
	ids.reserve(numTextures);
	filePaths.reserve(numTextures);

	for (const nlohmann::json& entry : data) {
		const save_load::IDType id = safeGet(entry, "id");
		const std::string filePath = safeGet(entry, "file_path");

		ids.push_back(id);
		filePaths.push_back(filePath);
	}

	return save_load::SerializedMeshes{
		.id = ids,
		.filePath = filePaths,
	};
}

save_load::SerializedMaterials loadMaterials(const nlohmann::json& data) {
	if (data.is_null()) return save_load::SerializedMaterials{};

	ASSERT(
		data.type() == nlohmann::json::value_t::array,
		"Material data must be an array of objects"
	);
	const size_t numTextures = data.size();

	std::vector<save_load::IDType> ids;
	std::vector<glm::vec3> speculars;
	std::vector<glm::vec3> diffuses;
	std::vector<glm::vec3> ambients;
	std::vector<glm::vec3> emissions;
	std::vector<float> shininesss;
	std::vector<std::optional<save_load::IDType>> albedoMaps;
	std::vector<std::optional<save_load::IDType>> normalMaps;
	std::vector<std::optional<save_load::IDType>> displacementMaps;
	std::vector<std::optional<save_load::IDType>> emissionMaps;
	std::vector<std::string> samplers;

	ids.reserve(numTextures);
	speculars.reserve(numTextures);
	diffuses.reserve(numTextures);
	ambients.reserve(numTextures);
	emissions.reserve(numTextures);
	shininesss.reserve(numTextures);
	albedoMaps.reserve(numTextures);
	normalMaps.reserve(numTextures);
	displacementMaps.reserve(numTextures);
	emissionMaps.reserve(numTextures);
	samplers.reserve(numTextures);

	for (const nlohmann::json& entry : data) {
		const save_load::IDType id = safeGet(entry, "id");
		const glm::vec3 specular = toVec3(safeGet(entry, "specular"));
		const glm::vec3 diffuse = toVec3(safeGet(entry, "diffuse"));
		const glm::vec3 ambient = toVec3(safeGet(entry, "ambient"));
		const glm::vec3 emission = toVec3(safeGet(entry, "emission"));
		const float shininess = safeGet(entry, "shininess");

		const std::optional<save_load::IDType> albedoMap =
			entry.contains("albedoMap") ? std::optional(entry["albedoMap"])
										: std::nullopt;
		const std::optional<save_load::IDType> normalMap =
			entry.contains("normalMap") ? std::optional(entry["normalMap"])
										: std::nullopt;
		const std::optional<save_load::IDType> displacementMap =
			entry.contains("displacementMap")
				? std::optional(entry["displacementMap"])
				: std::nullopt;
		const std::optional<save_load::IDType> emissionMap =
			entry.contains("emissionMap") ? std::optional(entry["emissionMap"])
										  : std::nullopt;

		const std::string sampler = safeGet(entry, "sampler");

		ids.push_back(id);
		speculars.push_back(specular);
		diffuses.push_back(diffuse);
		ambients.push_back(ambient);
		emissions.push_back(emission);
		shininesss.push_back(shininess);
		albedoMaps.push_back(albedoMap);
		normalMaps.push_back(normalMap);
		displacementMaps.push_back(displacementMap);
		emissionMaps.push_back(emissionMap);
		samplers.push_back(sampler);
	}

	return save_load::SerializedMaterials{
		.id = ids,
		.specular = speculars,
		.diffuse = diffuses,
		.ambient = ambients,
		.emission = emissions,
		.shininess = shininesss,
		.albedoMap = albedoMaps,
		.normalMap = normalMaps,
		.displacementMap = displacementMaps,
		.emissionMap = emissionMaps,
		.sampler = samplers
	};
}

save_load::SerializedStatics loadStatics(const nlohmann::json& data) {
	if (data.is_null()) return save_load::SerializedStatics{};

	ASSERT(
		data.type() == nlohmann::json::value_t::array,
		"Material data must be an array of objects"
	);

	std::vector<save_load::SerializedStatics::RegularData> regulars;
	std::vector<math::Transform> regularTransforms;
	std::vector<save_load::SerializedStatics::ObjData> objects;
	std::vector<math::Transform> objectTransforms;

	for (const nlohmann::json& entry : data) {
		const math::Transform transform =
			toTransform(safeGet(entry, "transform"));

		const save_load::SerializedStatics::Type type = [&]() {
			std::string value = safeGet(entry, "type");
			if (value == "obj") return save_load::SerializedStatics::Type::eObj;
			if (value == "regular")
				return save_load::SerializedStatics::Type::eRegular;
			LLOG_ERROR << "Type " << value << " is not valid";
			return save_load::SerializedStatics::Type::eRegular;
		}();

		switch (type) {
			case save_load::SerializedStatics::Type::eRegular: {
				const save_load::IDType material = safeGet(entry, "material");
				const save_load::IDType mesh = safeGet(entry, "mesh");
				regulars.push_back({.mesh = mesh, .material = material});
				regularTransforms.push_back(transform);
				break;
			}
			case save_load::SerializedStatics::Type::eObj: {
				const save_load::IDType id = safeGet(entry, "objID");
				objects.push_back({id});
				objectTransforms.push_back(transform);
				break;
			}
			default: __builtin_unreachable();
		};
	}

	return save_load::SerializedStatics{
		.regular = regulars,
		.regularTransform = regularTransforms,
		.object = objects,
		.objectTransform = objectTransforms,
	};
}

save_load::SerializedObjs loadObjects(const nlohmann::json& data) {
	if (data.is_null()) return save_load::SerializedObjs{};

	ASSERT(
		data.type() == nlohmann::json::value_t::array,
		"Objects data must be an array of objects"
	);

	const size_t numData = data.size();

	std::vector<save_load::IDType> ids;
	std::vector<std::string> modelPaths;
	std::vector<std::string> mtlPaths;
	std::vector<std::string> texturePaths;

	ids.reserve(numData);
	modelPaths.reserve(numData);
	mtlPaths.reserve(numData);
	texturePaths.reserve(numData);

	for (const nlohmann::json& entry : data) {
		const save_load::IDType id = safeGet(entry, "id");
		const std::string modelPath = safeGet(entry, "modelPath");
		const std::string mtlPath = safeGet(entry, "mtlPath");
		const std::string texturePath = safeGet(entry, "texturePath");

		ids.push_back(id);
		modelPaths.push_back(modelPath);
		mtlPaths.push_back(mtlPath);
		texturePaths.push_back(texturePath);
	}

	return {
		.id = ids,
		.modelPath = modelPaths,
		.mtlPath = mtlPaths,
		.texturePath = texturePaths
	};
}

}  // namespace

namespace save_load {

SerializedWorld JsonSerializer::loadWorld(std::string_view filePath) const {
	std::ifstream fileStream(filePath.data());

	const nlohmann::json data = nlohmann::json::parse(fileStream);

	fileStream.close();

	ASSERT(
		data.type() == nlohmann::json::value_t::object,
		"World data must be an objects, with fields: [textures, meshes, "
		"materials, statics]"
	);

	LLOG_INFO << "Json world from " << filePath << " successfully parsed";

	const save_load::SerializedTextures textures =
		loadTextures(safeGet(data, "textures"));
	LLOG_INFO << "Textures loaded from json";

	const save_load::SerializedMeshes meshes =
		loadMeshes(safeGet(data, "meshes"));
	LLOG_INFO << "Meshes loaded from json";

	const save_load::SerializedMaterials materials =
		loadMaterials(safeGet(data, "materials"));
	LLOG_INFO << "Materials loaded from json";

	const save_load::SerializedObjs objects =
		loadObjects(safeGet(data, "objects"));
	LLOG_INFO << "Objects loaded from json";

	const save_load::SerializedStatics statics =
		loadStatics(safeGet(data, "statics"));
	LLOG_INFO << "Statics loaded from json";

	return {
		.textures = textures,
		.meshes = meshes,
		.materials = materials,
		.objects = objects,
		.statics = statics
	};
}

}  // namespace save_load

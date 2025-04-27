#include "save_load/world_loader.h"

#include "core/logger/logger.h"
#include "core/math/transform.h"
#include "game_world/world.h"
#include "low_level_renderer/materials.h"

namespace {

using IndexMap = std::unordered_map<save_load::IDType, size_t>;

IndexMap extractIndexMap(std::span<const save_load::IDType> ids) {
	std::unordered_map<save_load::IDType, size_t> indexMap;
	indexMap.reserve(ids.size());
	for (size_t i = 0; i < ids.size(); i++) indexMap.emplace(ids[i], i);
	return indexMap;
}
};	// namespace
namespace save_load {

bool WorldLoader::isValid(const SerializedWorld& serializedWorld) const {
	const IndexMap textureIndexMap =
		extractIndexMap(serializedWorld.textures.id);
	const IndexMap materialIndexMap =
		extractIndexMap(serializedWorld.materials.id);
	const IndexMap meshIndexMap = extractIndexMap(serializedWorld.meshes.id);

	const size_t numMaterials = serializedWorld.materials.id.size();
	for (size_t i = 0; i < numMaterials; i++) {
		const IDType albedoID = serializedWorld.materials.albedoMap[i];
		if (!textureIndexMap.contains(albedoID)) {
			LLOG_ERROR << "albedo id (" << albedoID << ") is not defined";
			return false;
		}

		const IDType normalID = serializedWorld.materials.normalMap[i];
		if (!textureIndexMap.contains(normalID)) {
			LLOG_ERROR << "normal id (" << normalID << ") is not defined";
			return false;
		}

		const IDType displacementID =
			serializedWorld.materials.displacementMap[i];
		if (!textureIndexMap.contains(displacementID)) {
			LLOG_ERROR << "displacement id (" << displacementID
					   << ") is not defined";
			return false;
		}

		const IDType emissionID = serializedWorld.materials.emissionMap[i];
		if (!textureIndexMap.contains(emissionID)) {
			LLOG_ERROR << "emission id (" << emissionID << ") is not defined";
			return false;
		}

		const std::string_view samplerTypeAsString =
			serializedWorld.materials.sampler[i];
		if (samplerTypeAsString != "point" && samplerTypeAsString != "linear") {
			LLOG_ERROR << "sampler type " << samplerTypeAsString
					   << " is invalid";
			return false;
		}
	}

	const size_t numStatics = serializedWorld.statics.mesh.size();
	for (size_t i = 0; i < numStatics; i++) {
		const IDType meshID = serializedWorld.statics.mesh[i];
		if (!meshIndexMap.contains(meshID)) {
			LLOG_ERROR << "mesh id (" << meshID << ") is not defined";
			return false;
		}

		const IDType materialID = serializedWorld.statics.material[i];
		if (!materialIndexMap.contains(materialID)) {
			LLOG_ERROR << "material id (" << materialID << ") is not defined";
			return false;
		}
	}

	return true;
}

void WorldLoader::load(
	graphics::Module& graphics,
	game_world::World& world,
	const SerializedWorld& serializedWorld
) const {
	const IndexMap textureIndexMap =
		extractIndexMap(serializedWorld.textures.id);
	const IndexMap materialIndexMap =
		extractIndexMap(serializedWorld.materials.id);
	const IndexMap meshIndexMap = extractIndexMap(serializedWorld.meshes.id);

	const std::vector<graphics::TextureID> loadedTextures = [&]() {
		std::vector<graphics::TextureID> result;
		const size_t numTextures = serializedWorld.textures.id.size();
		result.reserve(numTextures);
		for (size_t i = 0; i < numTextures; i++) {
			result.push_back(graphics.loadTexture(
				serializedWorld.textures.filePath[i],
				serializedWorld.textures.format[i]
			));
		}
		return result;
	}();

	const std::vector<graphics::MeshID> loadedMeshes = [&]() {
		std::vector<graphics::MeshID> result;
		const size_t numMeshes = serializedWorld.meshes.id.size();
		result.reserve(numMeshes);
		for (size_t i = 0; i < numMeshes; i++) {
			result.push_back(
				graphics.loadMesh(serializedWorld.meshes.filePath[i])
			);
		}
		return result;
	}();

	const std::vector<graphics::MaterialInstanceID> loadedMaterials = [&]() {
		std::vector<graphics::MaterialInstanceID> result;
		const size_t numMaterials = serializedWorld.materials.id.size();
		result.reserve(numMaterials);
		for (size_t i = 0; i < numMaterials; i++) {
			const IDType albedoID = serializedWorld.materials.albedoMap[i];
			const IDType normalID = serializedWorld.materials.normalMap[i];
			const IDType displacementID =
				serializedWorld.materials.displacementMap[i];
			const IDType emissionID = serializedWorld.materials.emissionMap[i];

			const size_t albedoIndex = textureIndexMap.at(albedoID);
			const size_t normalIndex = textureIndexMap.at(normalID);
			const size_t displacementIndex = textureIndexMap.at(displacementID);
			const size_t emissionIndex = textureIndexMap.at(emissionID);

			const graphics::SamplerType sampler = [&]() {
				const std::string_view samplerTypeAsString =
					serializedWorld.materials.sampler[i];
				if (samplerTypeAsString == "linear")
					return graphics::SamplerType::eLinear;
				if (samplerTypeAsString == "point")
					return graphics::SamplerType::ePoint;
				__builtin_unreachable();
			}();

			result.push_back(graphics.loadMaterial(
				loadedTextures[albedoIndex],
				loadedTextures[normalIndex],
				loadedTextures[displacementIndex],
				loadedTextures[emissionIndex],
				graphics::MaterialProperties{
					.specular = serializedWorld.materials.specular[i],
					.diffuse = serializedWorld.materials.diffuse[i],
					.ambient = serializedWorld.materials.ambient[i],
					.emission = serializedWorld.materials.emission[i],
					.shininess = serializedWorld.materials.shininess[i]
				},
				sampler
			));
		}
		return result;
	}();

	LLOG_INFO << "All materials loaded into engine";

	const auto [transforms, materials, meshes] = [&]() {
		const size_t numStatics = serializedWorld.statics.transform.size();
		std::vector<glm::mat4> transforms;
		std::vector<graphics::MaterialInstanceID> materials;
		std::vector<graphics::MeshID> meshes;

		transforms.reserve(numStatics);
		materials.reserve(numStatics);
		meshes.reserve(numStatics);

		for (size_t i = 0; i < numStatics; i++) {
			transforms.push_back(
				math::toMat4(serializedWorld.statics.transform[i])
			);

			const IDType meshID = serializedWorld.statics.mesh[i];
			const size_t meshIndex = meshIndexMap.at(meshID);
			meshes.push_back(loadedMeshes[meshIndex]);

			const IDType materialID = serializedWorld.statics.material[i];
			const size_t materialIndex = materialIndexMap.at(materialID);
			materials.push_back(loadedMaterials[materialIndex]);
		}

		return std::make_tuple(transforms, materials, meshes);
	}();

	game_world::emplaceStatics(world, transforms, materials, meshes);
}
}  // namespace save_load

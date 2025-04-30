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
		const auto doesTextureExistOrUndefined =
			[&](const std::optional<IDType>& textureID) -> bool {
			if (!textureID || textureIndexMap.contains(textureID.value()))
				return true;
			LLOG_ERROR << "Texture id (" << textureID.value()
					   << ") is not defined";
			return false;
		};

		const bool isAnyTextureBad =
			!doesTextureExistOrUndefined(serializedWorld.materials.albedoMap[i]
			) ||
			!doesTextureExistOrUndefined(serializedWorld.materials.normalMap[i]
			) ||
			!doesTextureExistOrUndefined(
				serializedWorld.materials.displacementMap[i]
			) ||
			!doesTextureExistOrUndefined(
				serializedWorld.materials.emissionMap[i]
			);

		if (isAnyTextureBad) return false;

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

	const auto [loadedMaterials, variants] = [&]() {
		std::vector<graphics::MaterialInstanceID> materials;
		std::vector<graphics::PipelineSpecializationConstants> variants;

		const size_t numMaterials = serializedWorld.materials.id.size();
		materials.reserve(numMaterials);
		variants.reserve(numMaterials);
		for (size_t i = 0; i < numMaterials; i++) {
			const auto getTexture =
				[&](const std::optional<save_load::IDType>& id
				) -> std::optional<graphics::TextureID> {
				if (id.has_value()) {
					const size_t textureIndex = textureIndexMap.at(id.value());
					return std::optional(loadedTextures.at(textureIndex));
				}
				return std::nullopt;
			};

			const graphics::SamplerType sampler = [&]() {
				const std::string_view samplerTypeAsString =
					serializedWorld.materials.sampler[i];
				if (samplerTypeAsString == "linear")
					return graphics::SamplerType::eLinear;
				if (samplerTypeAsString == "point")
					return graphics::SamplerType::ePoint;
				__builtin_unreachable();
			}();

			const graphics::MaterialCreateInfo createInfo = {
				.albedo = getTexture(serializedWorld.materials.albedoMap[i]),
				.normal = getTexture(serializedWorld.materials.normalMap[i]),
				.displacement =
					getTexture(serializedWorld.materials.displacementMap[i]),
				.emission =
					getTexture(serializedWorld.materials.emissionMap[i]),
				.materialProperties =
					graphics::MaterialProperties{
						.specular = serializedWorld.materials.specular[i],
						.diffuse = serializedWorld.materials.diffuse[i],
						.ambient = serializedWorld.materials.ambient[i],
						.emission = serializedWorld.materials.emission[i],
						.shininess = serializedWorld.materials.shininess[i]
					},
				.sampler = sampler
			};

			const auto material = graphics.loadMaterial(createInfo);
			const auto variant =
				graphics::createSpecializationConstant(createInfo);

            graphics.createPipelineVariant(variant);

			materials.push_back(material);
			variants.push_back(variant);
		}
		return std::make_tuple(materials, variants);
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

	game_world::emplaceStatics(world, variants, transforms, materials, meshes);
}
}  // namespace save_load

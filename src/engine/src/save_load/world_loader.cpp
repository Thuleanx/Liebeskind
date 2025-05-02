#include "save_load/world_loader.h"

#include "core/logger/logger.h"
#include "core/math/transform.h"
#include "game_world/world.h"
#include "low_level_renderer/materials.h"
#include "resource_management/obj_loader.h"

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
	const IndexMap objectIndexMap = extractIndexMap(serializedWorld.objects.id);
	const IndexMap meshIndexMap = extractIndexMap(serializedWorld.meshes.id);

	{  // check size : texture
		const size_t numTextures = serializedWorld.textures.id.size();
		if (numTextures != serializedWorld.textures.format.size()) {
			LLOG_ERROR << "Number of formats ("
					   << serializedWorld.textures.format.size()
					   << ") does not match the number of textures ("
					   << numTextures << ")";
			return false;
		}

		if (numTextures != serializedWorld.textures.filePath.size()) {
			LLOG_ERROR << "Number of filepaths ("
					   << serializedWorld.textures.filePath.size()
					   << ") does not match the number of textures ("
					   << numTextures << ")";
			return false;
		}
	}

	{  // check size : mesh
		const size_t numMeshes = serializedWorld.meshes.id.size();
		if (numMeshes != serializedWorld.meshes.filePath.size()) {
			LLOG_ERROR << "Number of filepaths ("
					   << serializedWorld.meshes.filePath.size()
					   << ") does not match the number of meshes (" << numMeshes
					   << ")";
			return false;
		}
	}

	{  // check size : materials
		const size_t numMaterials = serializedWorld.materials.id.size();
		const std::array<size_t, 11> sizes = {
			serializedWorld.materials.id.size(),
			serializedWorld.materials.specular.size(),
			serializedWorld.materials.diffuse.size(),
			serializedWorld.materials.ambient.size(),
			serializedWorld.materials.emission.size(),
			serializedWorld.materials.shininess.size(),
			serializedWorld.materials.albedoMap.size(),
			serializedWorld.materials.normalMap.size(),
			serializedWorld.materials.displacementMap.size(),
			serializedWorld.materials.emissionMap.size(),
			serializedWorld.materials.sampler.size(),
		};
		for (size_t i = 0; i < sizes.size(); i++) {
			if (sizes[i] != numMaterials) {
				LLOG_ERROR << "Member variable at index " << i
						   << " in SerializedMaterials has mismatched size";
				return false;
			}
		}
	}

	{  // check size : objects
		const std::array<size_t, 3> sizes = {
			serializedWorld.objects.id.size(),
			serializedWorld.objects.modelPath.size(),
			serializedWorld.objects.mtlPath.size(),
		};
		for (size_t i = 0; i < sizes.size(); i++) {
			if (sizes[i] != sizes[0]) {
				LLOG_ERROR << "Member variable at index " << i
						   << " in SerializedObjs has mismatched size";
				return false;
			}
		}
	}

	{  // check size : statics
		if (serializedWorld.statics.object.size() !=
			serializedWorld.statics.objectTransform.size()) {
            LLOG_ERROR << "Mismatch object and object transform sizes";
			return false;
		}

		if (serializedWorld.statics.regular.size() !=
			serializedWorld.statics.regularTransform.size()) {
            LLOG_ERROR << "Mismatch regular and regular transform sizes";
			return false;
		}
	}

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

	const size_t numObjectStatics =
		serializedWorld.statics.objectTransform.size();
	for (size_t i = 0; i < numObjectStatics; i++) {
		const IDType objectID = serializedWorld.statics.object[i].id;
		if (!objectIndexMap.contains(objectID)) {
			LLOG_ERROR << "object id (" << objectID << ") is not defined";
			return false;
		}
	}

	const size_t numRegularStatics =
		serializedWorld.statics.regularTransform.size();
	for (size_t i = 0; i < numRegularStatics; i++) {
		const IDType meshID = serializedWorld.statics.regular[i].mesh;
		if (!meshIndexMap.contains(meshID)) {
			LLOG_ERROR << "mesh id (" << meshID << ") is not defined";
			return false;
		}

		const IDType materialID = serializedWorld.statics.regular[i].material;
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

	const auto [loadedMaterials, loadedMaterialVariants] = [&]() {
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

	const std::vector<resource_management::Model> loadedModels = [&]() {
		std::vector<resource_management::Model> result;

		const size_t numObj = serializedWorld.objects.id.size();
		result.reserve(numObj);

		for (size_t i = 0; i < numObj; i++) {
			result.push_back(resource_management::loadObj(
				graphics,
				serializedWorld.objects.modelPath[i],
				serializedWorld.objects.mtlPath[i],
				serializedWorld.objects.texturePath[i]
			));
		}

		return result;
	}();

	LLOG_INFO << "All materials loaded into engine";

	const auto [variants, transforms, materials, meshes] = [&]() {
		const size_t numRegulars =
			serializedWorld.statics.regularTransform.size();
		const size_t numObjects =
			serializedWorld.statics.objectTransform.size();
		const size_t numStatics = numRegulars + numObjects;
		std::vector<graphics::PipelineSpecializationConstants> variants;
		std::vector<glm::mat4> transforms;
		std::vector<graphics::MaterialInstanceID> materials;
		std::vector<graphics::MeshID> meshes;

		variants.reserve(numStatics);
		transforms.reserve(numStatics);
		materials.reserve(numStatics);
		meshes.reserve(numStatics);

		for (size_t i = 0; i < numRegulars; i++) {
			transforms.push_back(
				math::toMat4(serializedWorld.statics.regularTransform[i])
			);

			const IDType meshID = serializedWorld.statics.regular[i].mesh;
			const size_t meshIndex = meshIndexMap.at(meshID);
			meshes.push_back(loadedMeshes[meshIndex]);

			const IDType materialID =
				serializedWorld.statics.regular[i].material;
			const size_t materialIndex = materialIndexMap.at(materialID);
			variants.push_back(loadedMaterialVariants[materialIndex]);
			materials.push_back(loadedMaterials[materialIndex]);
		}

		for (size_t i = 0; i < numObjects; i++) {
			const resource_management::Model& model =
				loadedModels[serializedWorld.statics.object[i].id];
			const glm::mat4 transform =
				math::toMat4(serializedWorld.statics.objectTransform[i]);

			const size_t numParts = model.materials.size();
			for (size_t part = 0; part < numParts; part++) {
				transforms.push_back(transform);
				materials.push_back(model.materials[part]);
				variants.push_back(model.variants[part]);
				meshes.push_back(model.meshes[part]);
			}
		}

		return std::make_tuple(variants, transforms, materials, meshes);
	}();

	game_world::emplaceStatics(world, variants, transforms, materials, meshes);
}
}  // namespace save_load

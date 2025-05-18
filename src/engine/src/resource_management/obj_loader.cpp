#include "resource_management/obj_loader.h"

#include <tiny_obj_loader.h>

#include <unordered_map>

#include "core/logger/assert.h"

namespace resource_management {

Model loadObj(
	graphics::Module& graphics,
	std::string_view objPath,
	std::string_view mtlDir,
	std::string_view texturePath
) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	const bool successfullyLoadedModel = tinyobj::LoadObj(
		&attrib, &shapes, &materials, &warn, &err, objPath.data(), mtlDir.data()
	);

	ASSERT(
		successfullyLoadedModel,
		"Can't load model at " << objPath << " and materials at " << mtlDir
							   << " " << warn << " " << err
	);

	const size_t numMaterials = materials.size();

	// We will be grouping these meshes so that it's one mesh per material
	std::vector<graphics::PipelineSpecializationConstants> variants;
	std::vector<graphics::MeshID> loadedMeshes;
	std::vector<graphics::MaterialInstanceID> loadedMaterials;
	variants.resize(numMaterials);
	loadedMeshes.resize(numMaterials);
	loadedMaterials.resize(numMaterials);

	struct PerMaterialData {
		std::vector<graphics::Vertex> vertices;
		std::vector<graphics::IndexType> indices;
		std::unordered_map<graphics::Vertex, size_t> uniqueVertices;
	};

	// We want to compute tangents of the same vertex globally. Sometimes the
	// same vertices belong to a different mesh since we split on material
	std::vector<glm::vec3> globalTangents;
	std::unordered_map<graphics::Vertex, size_t> uniqueGlobalVertices;

	std::vector<PerMaterialData> materialDatas;
	materialDatas.resize(numMaterials);

	for (const tinyobj::shape_t& shape : shapes) {
		ASSERT(
			shape.mesh.material_ids.size() * 3 == shape.mesh.indices.size(),
			"Not a mesh made of triangles"
		);

		ASSERT(
			shape.mesh.material_ids.size() ==
				shape.mesh.num_face_vertices.size(),
			"Materials " << shape.mesh.material_ids.size()
						 << " and num vertices arrays "
						 << shape.mesh.num_face_vertices.size()
						 << " are not of the same size"
		);

		size_t faceIndexOffset = 0;
		for (size_t face = 0; face < shape.mesh.num_face_vertices.size();
			 face++) {
			ASSERT(
				shape.mesh.num_face_vertices[face] == 3,
				"Cannot support meshes with polygons of "
					<< shape.mesh.num_face_vertices[face]
					<< " vertices, only triangle meshes permitted"
			);

			const int perFaceMaterialIndex = shape.mesh.material_ids[face];
			const size_t numVertices = shape.mesh.num_face_vertices[face];

			PerMaterialData& faceMaterialData =
				materialDatas[perFaceMaterialIndex];

			for (size_t faceIndex = 0; faceIndex < numVertices; faceIndex++) {
				const tinyobj::index_t index =
					shape.mesh.indices[faceIndexOffset + faceIndex];
				ASSERT(
					index.normal_index >= 0, "Vertex does not specify a normal"
				);
				ASSERT(
					index.texcoord_index >= 0,
					"Vertex does not specify a tex coordinate"
				);

				const graphics::Vertex vertex{
					.position =
						glm::vec3{
							attrib.vertices[3 * index.vertex_index + 0],
							attrib.vertices[3 * index.vertex_index + 1],
							attrib.vertices[3 * index.vertex_index + 2]
						},
					.normal =
						glm::vec3{
							attrib.normals[3 * index.normal_index + 0],
							attrib.normals[3 * index.normal_index + 1],
							attrib.normals[3 * index.normal_index + 2]
						},
					.tangent = glm::vec3(0),
					.color = glm::vec3{1.0, 1.0, 1.0},
					// obj format coordinate system makes 0 the bottom of
					// the image, which is different from vulkan which
					// considers it the top
					.texCoord =
						glm::vec2{
							attrib.texcoords[2 * index.texcoord_index],
							1.0 - attrib.texcoords[2 * index.texcoord_index + 1]
						}
				};

				// insert in global vertex map if needed
				if (!uniqueGlobalVertices.count(vertex)) {
					const size_t vertexIndex = uniqueGlobalVertices.size();
					uniqueGlobalVertices[vertex] = vertexIndex;
					globalTangents.emplace_back(0);
				}

				const size_t vertexIndex = [&]() {
					if (!faceMaterialData.uniqueVertices.count(vertex)) {
						const size_t result =
							faceMaterialData.uniqueVertices.size();
						faceMaterialData.vertices.push_back(vertex);
						faceMaterialData.uniqueVertices[vertex] = result;
						return result;
					}
					return faceMaterialData.uniqueVertices[vertex];
				}();

				faceMaterialData.indices.push_back(vertexIndex);
			}

			const size_t faceMaterialIndex =
				faceMaterialData.indices.size() - numVertices;

			const graphics::Vertex& v0 =
				faceMaterialData
					.vertices[faceMaterialData.indices[faceMaterialIndex]];
			const graphics::Vertex& v1 =
				faceMaterialData
					.vertices[faceMaterialData.indices[faceMaterialIndex + 1]];
			const graphics::Vertex& v2 =
				faceMaterialData
					.vertices[faceMaterialData.indices[faceMaterialIndex + 2]];

			const glm::vec3 tangent = getTangent(v0, v1, v2);

			globalTangents.at(uniqueGlobalVertices[v0]) += tangent;
			globalTangents.at(uniqueGlobalVertices[v1]) += tangent;
			globalTangents.at(uniqueGlobalVertices[v2]) += tangent;

			faceIndexOffset += numVertices;
		}

		ASSERT(
			faceIndexOffset == shape.mesh.indices.size(),
			"Have not parsed through all faces"
		);
	}

	for (glm::vec3& tangent : globalTangents) tangent = glm::normalize(tangent);

	std::unordered_map<std::string, graphics::Texture> textures;
	for (size_t materialId = 0; materialId < numMaterials; materialId++) {
		LLOG_INFO << "Loading material : " << materialId;
		for (graphics::Vertex& vertex : materialDatas[materialId].vertices)
			vertex.tangent = globalTangents[uniqueGlobalVertices[vertex]];

		PerMaterialData materialData = materialDatas[materialId];
		const graphics::MeshID mesh =
			graphics.loadMesh(materialData.vertices, materialData.indices);

		loadedMeshes[materialId] = mesh;

		const tinyobj::material_t material = materials[materialId];

		const graphics::MaterialProperties properties{
			.specular = reinterpret_cast<const glm::vec3&>(material.specular),
			.diffuse = reinterpret_cast<const glm::vec3&>(material.diffuse),
			.ambient = reinterpret_cast<const glm::vec3&>(material.ambient),
			.emission = reinterpret_cast<const glm::vec3&>(material.emission),
			.shininess = material.shininess,
		};

		const auto generateTexture = [&](const std::string& path,
										 graphics::TextureFormatHint formatHint
									 ) -> std::optional<graphics::TextureID> {
			if (path.length() > 0)
				return graphics.loadTexture(
					std::string(texturePath) + path, formatHint
				);
			return std::nullopt;
		};

		const std::optional<graphics::TextureID> albedo = generateTexture(
			material.diffuse_texname, graphics::TextureFormatHint::eGamma8
		);
		const std::optional<graphics::TextureID> normal = generateTexture(
			material.normal_texname, graphics::TextureFormatHint::eLinear8
		);
		const std::optional<graphics::TextureID> displacement = generateTexture(
			material.displacement_texname, graphics::TextureFormatHint::eLinear8
		);
		const std::optional<graphics::TextureID> emission = std::nullopt;

		const graphics::MaterialCreateInfo createInfo{
			.albedo = albedo,
			.normal = normal,
			.displacement = displacement,
			.emission = emission,
			.materialProperties = properties,
			.sampler = graphics::SamplerType::eLinear
		};

		loadedMaterials[materialId] = graphics.loadMaterial(createInfo);
		variants[materialId] =
			graphics::createSpecializationConstant(createInfo);
	}

	return {
		.variants = variants,
		.meshes = loadedMeshes,
		.materials = loadedMaterials
	};
}

};	// namespace resource_management

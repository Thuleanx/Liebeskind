#include "game_world/world.h"

#include "core/logger/assert.h"

namespace game_world {

void emplaceStatics(
	World& world,
	std::span<const glm::mat4> transforms,
	std::span<const graphics::MaterialInstanceID> materials,
	std::span<const graphics::MeshID> meshes
) {
	ASSERT(
		(transforms.size() == materials.size()) &&
			(transforms.size() == meshes.size()),
		"Emplacing static objects but the number of elements for each field is "
		"mismatched: "
			<< transforms.size() << " transforms; " << materials.size()
			<< " materials; " << meshes.size() << " meshes"
	);
	world.statics.transform.insert(
		world.statics.transform.end(), transforms.begin(), transforms.end()
	);
	world.statics.material.insert(
		world.statics.material.end(), materials.begin(), materials.end()
	);
	world.statics.mesh.insert(
		world.statics.mesh.end(), meshes.begin(), meshes.end()
	);
}

void addToSceneGraph(const World& world, scene_graph::Module& sceneGraph) {
	const std::vector<graphics::RenderObject> renderObjects = [&]() {
		std::vector<graphics::RenderObject> result;
		const size_t numStatics = world.statics.transform.size();
		result.reserve(numStatics);
		for (size_t i = 0; i < numStatics; i++) {
			result.emplace_back(
				world.statics.transform[i],
				world.statics.material[i],
				world.statics.mesh[i]
			);
		}
		return result;
	}();
    sceneGraph.addObjects(renderObjects);
}

}  // namespace game_world

#pragma once

#include "resource_management/mesh_manager.h"
#include "resource_management/shader_manager.h"

struct ResourceManager {
    MeshManager meshes;
    ShaderManager shaders;
};

#pragma once

#include "resource_management/material_manager.h"
#include "resource_management/mesh_manager.h"
#include "resource_management/shader_manager.h"
#include "resource_management/texture_manager.h"

struct ResourceManager {
    MaterialManager materials;
    MeshManager meshes;
    ShaderManager shaders;
    TextureManager textures;
};

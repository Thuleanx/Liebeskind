#pragma once

#include <vulkan/vulkan.hpp>

#include "low_level_renderer/vertex_buffer.h"

// TODO: improve with generation counter
struct MeshID {
    uint32_t index;
};

struct Mesh {
    VertexBuffer vertexBuffer;
};

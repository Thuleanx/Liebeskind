#pragma once

#include "low_level_renderer/storage_buffer.h"
#include <glm/fwd.hpp>

struct InstanceData {
    StorageBuffer<glm::mat4> transforms;
};

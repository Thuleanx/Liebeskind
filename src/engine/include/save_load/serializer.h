#pragma once

#include "save_load/serialized_world.h"

namespace save_load {

struct Serializer {
   public:
    [[nodiscard]]
	virtual SerializedWorld loadWorld(std::string_view filePath) const = 0;
};

};	// namespace save_load

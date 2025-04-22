#pragma once

#include "save_load/serializer.h"

namespace save_load {
struct JsonSerializer : Serializer {
   public:
    [[nodiscard]]
	virtual SerializedWorld loadWorld(std::string_view filePath) const;
};
}  // namespace save_load

#include "core/file_system/file.h"

#include <fstream>

#include "core/logger/logger.h"

namespace file_system {
std::optional<std::vector<char>> readFile(std::string_view fileName) {
	std::ifstream file(fileName.data(), std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		LLOG_ERROR << "Can't open file " << fileName;
		return {};
	}

	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(fileSize);
    buffer.reserve(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}
}  // namespace file_system

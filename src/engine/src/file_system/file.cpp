#include "file_system/file.h"

#include "logger/logger.h"
#include <fstream>

namespace FileUtilities {
    std::optional<std::vector<char>> readFile(const std::string& fileName) {
        std::ifstream file(fileName, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            LLOG_ERROR << "Can't open file " << fileName;
            return {};
        }

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }
}

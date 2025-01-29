#pragma once

#include <vector>
#include <string>
#include <optional>

namespace FileUtilities {
    std::optional<std::vector<char>> readFile(const std::string& fileName);
}


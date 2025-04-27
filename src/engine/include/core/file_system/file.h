#pragma once

#include <vector>
#include <string_view>
#include <optional>

namespace file_system {
    std::optional<std::vector<char>> readFile(std::string_view fileName);
}


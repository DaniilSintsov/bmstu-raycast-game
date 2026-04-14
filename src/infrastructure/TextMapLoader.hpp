#pragma once

#include <filesystem>

#include "domain/Map.hpp"

namespace doomlike::infrastructure {

class TextMapLoader {
public:
    [[nodiscard]] static domain::Map loadFromFile(const std::filesystem::path& path);
};

}  // namespace doomlike::infrastructure


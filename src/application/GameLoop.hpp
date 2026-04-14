#pragma once

#include <filesystem>

namespace doomlike::presentation {
class SdlApp;
}

namespace doomlike::application {

class GameLoop {
public:
    int run(const std::filesystem::path& mapPath);
};

}  // namespace doomlike::application


#include <filesystem>
#include <iostream>
#include <string_view>

#include <SDL3/SDL_main.h>

#include "application/GameLoop.hpp"

int main(int argc, char** argv)
{
    try {
        std::filesystem::path mapPath = std::filesystem::path(DOOMLIKE_PROJECT_SOURCE_DIR) / "resources" / "maps" /
                                        "arena.txt";
        if (argc > 1) {
            mapPath = argv[1];
        }

        doomlike::application::GameLoop loop;
        return loop.run(mapPath);
    } catch (const std::exception& exception) {
        std::cerr << "Fatal error: " << exception.what() << '\n';
        return 1;
    }
}

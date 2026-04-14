#include "infrastructure/TextMapLoader.hpp"

#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace doomlike::infrastructure {

namespace {

[[nodiscard]] bool isAllowedTile(const char tile)
{
    return tile == '#' || tile == '.' || tile == '1' || tile == '2';
}

}  // namespace

domain::Map TextMapLoader::loadFromFile(const std::filesystem::path& path)
{
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Не удалось открыть файл карты: " + path.string());
    }

    std::vector<std::string> lines;
    std::string line;

    while (std::getline(input, line)) {
        if (line.empty() || line[0] == ';') {
            continue;
        }
        lines.push_back(line);
    }

    if (lines.empty()) {
        throw std::runtime_error("Файл карты пуст: " + path.string());
    }

    const int height = static_cast<int>(lines.size());
    const int width = static_cast<int>(lines.front().size());

    for (const std::string& row : lines) {
        if (static_cast<int>(row.size()) != width) {
            throw std::runtime_error("Карта должна быть прямоугольной");
        }
    }

    domain::Map map(path.stem().string(), width, height, '.');
    bool hostSpawnFound = false;
    bool clientSpawnFound = false;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const char tile = lines[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
            if (!isAllowedTile(tile)) {
                throw std::runtime_error("Недопустимый символ в карте: " + std::string(1, tile));
            }

            const bool border = x == 0 || y == 0 || x == width - 1 || y == height - 1;
            if (border && tile != '#') {
                throw std::runtime_error("Внешняя граница карты должна быть из стен '#'");
            }

            switch (tile) {
                case '1':
                    map.setSpawn(domain::PlayerId::Host, {static_cast<float>(x) + 0.5F, static_cast<float>(y) + 0.5F});
                    map.setTile(x, y, '.');
                    hostSpawnFound = true;
                    break;
                case '2':
                    map.setSpawn(domain::PlayerId::Client, {static_cast<float>(x) + 0.5F, static_cast<float>(y) + 0.5F});
                    map.setTile(x, y, '.');
                    clientSpawnFound = true;
                    break;
                default:
                    map.setTile(x, y, tile);
                    break;
            }
        }
    }

    if (!hostSpawnFound || !clientSpawnFound) {
        throw std::runtime_error("На карте должны быть точки старта '1' и '2'");
    }

    return map;
}

}  // namespace doomlike::infrastructure


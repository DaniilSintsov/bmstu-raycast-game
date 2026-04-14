#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <vector>

#include "domain/Math.hpp"
#include "domain/Player.hpp"

namespace doomlike::domain {

class Map {
public:
    Map() = default;
    Map(std::string name, int width, int height, char fill = '.');

    [[nodiscard]] const std::string& name() const noexcept;
    [[nodiscard]] int width() const noexcept;
    [[nodiscard]] int height() const noexcept;

    [[nodiscard]] bool isInside(int x, int y) const noexcept;
    [[nodiscard]] char tileAt(int x, int y) const noexcept;
    [[nodiscard]] bool isWall(int x, int y) const noexcept;

    void setTile(int x, int y, char tile);
    void setSpawn(PlayerId id, Vec2 position);

    [[nodiscard]] bool hasSpawn(PlayerId id) const noexcept;
    [[nodiscard]] Vec2 spawnFor(PlayerId id) const;

private:
    [[nodiscard]] std::size_t index(int x, int y) const noexcept;

    std::string name_ {"unnamed"};
    int width_ {0};
    int height_ {0};
    std::vector<char> tiles_ {};
    std::array<Vec2, 2> spawns_ {};
    std::array<bool, 2> spawnPresent_ {false, false};
};

}  // namespace doomlike::domain


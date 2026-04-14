#include "domain/Map.hpp"

#include <stdexcept>

namespace doomlike::domain {

Map::Map(std::string name, const int width, const int height, const char fill)
    : name_(std::move(name)),
      width_(width),
      height_(height),
      tiles_(static_cast<std::size_t>(width * height), fill)
{
}

const std::string& Map::name() const noexcept
{
    return name_;
}

int Map::width() const noexcept
{
    return width_;
}

int Map::height() const noexcept
{
    return height_;
}

bool Map::isInside(const int x, const int y) const noexcept
{
    return x >= 0 && y >= 0 && x < width_ && y < height_;
}

char Map::tileAt(const int x, const int y) const noexcept
{
    if (!isInside(x, y)) {
        return '#';
    }
    return tiles_[index(x, y)];
}

bool Map::isWall(const int x, const int y) const noexcept
{
    return tileAt(x, y) == '#';
}

void Map::setTile(const int x, const int y, const char tile)
{
    if (!isInside(x, y)) {
        throw std::out_of_range("Map::setTile out of bounds");
    }
    tiles_[index(x, y)] = tile;
}

void Map::setSpawn(const PlayerId id, const Vec2 position)
{
    spawns_[toIndex(id)] = position;
    spawnPresent_[toIndex(id)] = true;
}

bool Map::hasSpawn(const PlayerId id) const noexcept
{
    return spawnPresent_[toIndex(id)];
}

Vec2 Map::spawnFor(const PlayerId id) const
{
    if (!hasSpawn(id)) {
        throw std::runtime_error("Spawn point is missing");
    }
    return spawns_[toIndex(id)];
}

std::size_t Map::index(const int x, const int y) const noexcept
{
    return static_cast<std::size_t>((y * width_) + x);
}

}  // namespace doomlike::domain


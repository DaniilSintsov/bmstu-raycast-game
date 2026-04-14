#include "domain/CollisionSystem.hpp"

#include <array>
#include <cmath>

namespace doomlike::domain {

bool CollisionSystem::canOccupy(const Map& map, const Vec2 position, const float radius)
{
    const std::array<Vec2, 5> probes {
        Vec2 {position.x - radius, position.y - radius},
        Vec2 {position.x + radius, position.y - radius},
        Vec2 {position.x - radius, position.y + radius},
        Vec2 {position.x + radius, position.y + radius},
        position,
    };

    for (const Vec2& probe : probes) {
        const int cellX = static_cast<int>(std::floor(probe.x));
        const int cellY = static_cast<int>(std::floor(probe.y));
        if (map.isWall(cellX, cellY)) {
            return false;
        }
    }

    return true;
}

Vec2 CollisionSystem::resolveMovement(const Map& map, const Vec2 from, const Vec2 desired, const float radius)
{
    Vec2 resolved = from;

    const Vec2 xOnly {desired.x, from.y};
    if (canOccupy(map, xOnly, radius)) {
        resolved.x = desired.x;
    }

    const Vec2 yOnly {resolved.x, desired.y};
    if (canOccupy(map, yOnly, radius)) {
        resolved.y = desired.y;
    }

    return resolved;
}

}  // namespace doomlike::domain

#pragma once

#include "domain/Map.hpp"
#include "domain/Math.hpp"

namespace doomlike::domain {

class CollisionSystem {
public:
    [[nodiscard]] static bool canOccupy(const Map& map, Vec2 position, float radius);
    [[nodiscard]] static Vec2 resolveMovement(const Map& map, Vec2 from, Vec2 desired, float radius);
};

}  // namespace doomlike::domain


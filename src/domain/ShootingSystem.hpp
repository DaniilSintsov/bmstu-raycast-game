#pragma once

#include <optional>

#include "domain/World.hpp"

namespace doomlike::domain {

struct ShotResult {
    PlayerId shooter {PlayerId::Host};
    PlayerId target {PlayerId::Client};
    Vec2 hitPosition {};
    float distance {0.0F};
};

class ShootingSystem {
public:
    [[nodiscard]] static std::optional<ShotResult> fireHitscan(const World& world, PlayerId shooterId);

private:
    [[nodiscard]] static bool hasLineOfSight(const Map& map, Vec2 from, Vec2 to);
};

}  // namespace doomlike::domain


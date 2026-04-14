#include "domain/ShootingSystem.hpp"

#include <algorithm>
#include <cmath>

namespace doomlike::domain {

std::optional<ShotResult> ShootingSystem::fireHitscan(const World& world, const PlayerId shooterId)
{
    const PlayerState& shooter = world.player(shooterId);
    const PlayerState& target = world.enemyOf(shooterId);

    if (!shooter.alive || !target.alive) {
        return std::nullopt;
    }

    const Vec2 delta = target.position - shooter.position;
    const float distance = length(delta);
    if (distance > shooter.weapon.range || distance <= 0.0001F) {
        return std::nullopt;
    }

    const float angleToTarget = angleBetween(shooter.position, target.position);
    const float angleDelta = std::abs(wrapAngle(angleToTarget - shooter.angleRadians));
    const float allowedAngle = std::atan2(target.radius, std::max(distance, 0.001F)) + degToRad(1.5F);

    if (angleDelta > allowedAngle) {
        return std::nullopt;
    }

    if (!hasLineOfSight(world.map(), shooter.position, target.position)) {
        return std::nullopt;
    }

    return ShotResult {
        .shooter = shooterId,
        .target = target.id,
        .hitPosition = target.position,
        .distance = distance,
    };
}

bool ShootingSystem::hasLineOfSight(const Map& map, const Vec2 from, const Vec2 to)
{
    const Vec2 segment = to - from;
    const float distance = length(segment);
    if (distance <= 0.0001F) {
        return true;
    }

    const Vec2 step = normalize(segment) * 0.05F;
    Vec2 sample = from;
    float travelled = 0.0F;

    while (travelled < distance) {
        sample += step;
        travelled += 0.05F;
        if (map.isWall(static_cast<int>(std::floor(sample.x)), static_cast<int>(std::floor(sample.y)))) {
            return false;
        }
    }

    return true;
}

}  // namespace doomlike::domain

#include "presentation/Raycaster.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace doomlike::presentation {

Raycaster::Raycaster(const float fovRadians, const float maxDistance)
    : fovRadians_(fovRadians),
      maxDistance_(maxDistance)
{
}

std::vector<RayHit> Raycaster::castColumns(
    const domain::Map& map,
    const domain::Vec2 position,
    const float viewAngleRadians,
    const int screenWidth
) const
{
    if (screenWidth <= 0) {
        return {};
    }

    std::vector<RayHit> columns(static_cast<std::size_t>(screenWidth));

    for (int x = 0; x < screenWidth; ++x) {
        const float t = (static_cast<float>(x) + 0.5F) / static_cast<float>(screenWidth);
        const float rayAngle = (viewAngleRadians - (fovRadians_ * 0.5F)) + (fovRadians_ * t);
        const float rayDirX = std::cos(rayAngle);
        const float rayDirY = std::sin(rayAngle);

        int mapX = static_cast<int>(position.x);
        int mapY = static_cast<int>(position.y);

        const float deltaDistX =
            std::abs(rayDirX) < 0.0001F ? std::numeric_limits<float>::infinity() : std::abs(1.0F / rayDirX);
        const float deltaDistY =
            std::abs(rayDirY) < 0.0001F ? std::numeric_limits<float>::infinity() : std::abs(1.0F / rayDirY);

        int stepX = 0;
        int stepY = 0;
        float sideDistX = 0.0F;
        float sideDistY = 0.0F;

        if (rayDirX < 0.0F) {
            stepX = -1;
            sideDistX = (position.x - static_cast<float>(mapX)) * deltaDistX;
        } else {
            stepX = 1;
            sideDistX = (static_cast<float>(mapX + 1) - position.x) * deltaDistX;
        }

        if (rayDirY < 0.0F) {
            stepY = -1;
            sideDistY = (position.y - static_cast<float>(mapY)) * deltaDistY;
        } else {
            stepY = 1;
            sideDistY = (static_cast<float>(mapY + 1) - position.y) * deltaDistY;
        }

        bool hit = false;
        int side = 0;
        float distance = maxDistance_;

        while (!hit) {
            if (sideDistX < sideDistY) {
                sideDistX += deltaDistX;
                mapX += stepX;
                side = 0;
            } else {
                sideDistY += deltaDistY;
                mapY += stepY;
                side = 1;
            }

            if (!map.isInside(mapX, mapY)) {
                break;
            }

            if (map.isWall(mapX, mapY)) {
                hit = true;
                const float rawDistance = side == 0 ? (sideDistX - deltaDistX) : (sideDistY - deltaDistY);
                distance = rawDistance * std::cos(rayAngle - viewAngleRadians);
                distance = std::clamp(distance, 0.0001F, maxDistance_);
            }
        }

        columns[static_cast<std::size_t>(x)] = RayHit {
            .hit = hit,
            .distance = distance,
            .cellX = mapX,
            .cellY = mapY,
            .side = side,
        };
    }

    return columns;
}

float Raycaster::fovRadians() const noexcept
{
    return fovRadians_;
}

}  // namespace doomlike::presentation


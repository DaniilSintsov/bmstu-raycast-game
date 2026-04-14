#pragma once

#include <vector>

#include "domain/Map.hpp"

namespace doomlike::presentation {

struct RayHit {
    bool hit {false};
    float distance {0.0F};
    int cellX {-1};
    int cellY {-1};
    int side {0};
};

class Raycaster {
public:
    explicit Raycaster(float fovRadians = domain::degToRad(60.0F), float maxDistance = 30.0F);

    [[nodiscard]] std::vector<RayHit> castColumns(
        const domain::Map& map,
        domain::Vec2 position,
        float viewAngleRadians,
        int screenWidth
    ) const;

    [[nodiscard]] float fovRadians() const noexcept;

private:
    float fovRadians_ {domain::degToRad(60.0F)};
    float maxDistance_ {30.0F};
};

}  // namespace doomlike::presentation


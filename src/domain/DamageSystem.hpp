#pragma once

#include "domain/World.hpp"

namespace doomlike::domain {

class DamageSystem {
public:
    static void applyDamage(World& world, PlayerId targetId, int amount);
};

}  // namespace doomlike::domain


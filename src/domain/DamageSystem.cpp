#include "domain/DamageSystem.hpp"

#include <algorithm>

namespace doomlike::domain {

void DamageSystem::applyDamage(World& world, const PlayerId targetId, const int amount)
{
    PlayerState& target = world.player(targetId);
    if (!target.alive) {
        return;
    }

    target.hp = std::max(0, target.hp - amount);
    target.alive = target.hp > 0;

    if (!target.alive) {
        world.setWinner(otherPlayer(targetId));
        world.setPhase(MatchPhase::GameOver);
    }
}

}  // namespace doomlike::domain


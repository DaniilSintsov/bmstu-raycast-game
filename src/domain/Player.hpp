#pragma once

#include <cstddef>
#include <cstdint>

#include "domain/Math.hpp"

namespace doomlike::domain {

enum class PlayerId : std::uint8_t {
    Host = 0,
    Client = 1,
};

inline constexpr std::size_t toIndex(const PlayerId id) noexcept
{
    return static_cast<std::size_t>(id);
}

inline constexpr PlayerId otherPlayer(const PlayerId id) noexcept
{
    return id == PlayerId::Host ? PlayerId::Client : PlayerId::Host;
}

inline constexpr int kStartingHp = 100;
inline constexpr int kShotDamage = 10;

struct WeaponState {
    int damage {kShotDamage};
    float range {16.0F};
    float cooldownSeconds {0.25F};
    float cooldownRemaining {0.0F};
};

struct PlayerState {
    PlayerId id {PlayerId::Host};
    Vec2 position {1.5F, 1.5F};
    float angleRadians {0.0F};
    float radius {0.20F};
    int hp {kStartingHp};
    bool alive {true};
    WeaponState weapon {};
};

}  // namespace doomlike::domain


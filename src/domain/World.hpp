#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include "domain/Map.hpp"
#include "domain/Player.hpp"

namespace doomlike::domain {

enum class MatchPhase : std::uint8_t {
    Menu = 0,
    WaitingForPlayers = 1,
    Running = 2,
    GameOver = 3,
    OpponentDisconnected = 4,
};

class World {
public:
    World();

    void setMap(Map map);
    void resetForNewRound();

    [[nodiscard]] Map& map() noexcept;
    [[nodiscard]] const Map& map() const noexcept;

    [[nodiscard]] PlayerState& player(PlayerId id) noexcept;
    [[nodiscard]] const PlayerState& player(PlayerId id) const noexcept;
    [[nodiscard]] PlayerState& enemyOf(PlayerId id) noexcept;
    [[nodiscard]] const PlayerState& enemyOf(PlayerId id) const noexcept;

    void setPhase(MatchPhase phase) noexcept;
    [[nodiscard]] MatchPhase phase() const noexcept;

    void setWinner(std::optional<PlayerId> winner) noexcept;
    [[nodiscard]] const std::optional<PlayerId>& winner() const noexcept;

    void advanceTick() noexcept;
    [[nodiscard]] std::uint32_t tick() const noexcept;

private:
    void initializePlayersFromMap();

    Map map_ {};
    std::array<PlayerState, 2> players_ {};
    MatchPhase phase_ {MatchPhase::Menu};
    std::optional<PlayerId> winner_ {};
    std::uint32_t tick_ {0};
};

}  // namespace doomlike::domain


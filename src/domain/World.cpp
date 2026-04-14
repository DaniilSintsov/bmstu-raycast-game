#include "domain/World.hpp"

namespace doomlike::domain {

World::World()
{
    players_[0].id = PlayerId::Host;
    players_[1].id = PlayerId::Client;
}

void World::setMap(Map map)
{
    map_ = std::move(map);
    initializePlayersFromMap();
}

void World::resetForNewRound()
{
    initializePlayersFromMap();
    winner_.reset();
    tick_ = 0;
}

Map& World::map() noexcept
{
    return map_;
}

const Map& World::map() const noexcept
{
    return map_;
}

PlayerState& World::player(const PlayerId id) noexcept
{
    return players_[toIndex(id)];
}

const PlayerState& World::player(const PlayerId id) const noexcept
{
    return players_[toIndex(id)];
}

PlayerState& World::enemyOf(const PlayerId id) noexcept
{
    return player(otherPlayer(id));
}

const PlayerState& World::enemyOf(const PlayerId id) const noexcept
{
    return player(otherPlayer(id));
}

void World::setPhase(const MatchPhase phase) noexcept
{
    phase_ = phase;
}

MatchPhase World::phase() const noexcept
{
    return phase_;
}

void World::setWinner(const std::optional<PlayerId> winner) noexcept
{
    winner_ = winner;
}

const std::optional<PlayerId>& World::winner() const noexcept
{
    return winner_;
}

void World::advanceTick() noexcept
{
    ++tick_;
}

std::uint32_t World::tick() const noexcept
{
    return tick_;
}

void World::initializePlayersFromMap()
{
    player(PlayerId::Host) = PlayerState {PlayerId::Host, map_.spawnFor(PlayerId::Host), 0.0F};
    player(PlayerId::Client) = PlayerState {PlayerId::Client, map_.spawnFor(PlayerId::Client), 3.14159265F};
}

}  // namespace doomlike::domain


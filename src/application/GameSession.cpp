#include "application/GameSession.hpp"

#include <algorithm>
#include <cmath>

#include "domain/CollisionSystem.hpp"
#include "domain/DamageSystem.hpp"
#include "domain/ShootingSystem.hpp"
#include "infrastructure/TextMapLoader.hpp"

namespace doomlike::application {

bool GameSession::loadMap(const std::filesystem::path& mapPath)
{
    world_.setMap(infrastructure::TextMapLoader::loadFromFile(mapPath));
    loadedMapPath_ = mapPath;
    return true;
}

void GameSession::startOfflineDemo()
{
    world_.resetForNewRound();
    world_.setPhase(domain::MatchPhase::Running);
}

void GameSession::startWaitingForPlayers()
{
    world_.resetForNewRound();
    world_.setPhase(domain::MatchPhase::WaitingForPlayers);
}

void GameSession::restart()
{
    if (!loadedMapPath_.empty()) {
        world_.setMap(infrastructure::TextMapLoader::loadFromFile(loadedMapPath_));
    }
    world_.resetForNewRound();
    world_.setPhase(domain::MatchPhase::Running);
}

void GameSession::setLocalPlayerId(const domain::PlayerId playerId) noexcept
{
    localPlayerId_ = playerId;
}

void GameSession::update(const float deltaSeconds, const InputState& input)
{
    shotFiredThisFrame_ = false;
    damageDealtThisFrame_ = 0;
    updateWeaponCooldown(deltaSeconds);

    if (world_.phase() == domain::MatchPhase::GameOver || world_.phase() == domain::MatchPhase::OpponentDisconnected) {
        if (input.restartPressed) {
            restart();
        }
        return;
    }

    if (world_.phase() != domain::MatchPhase::Running) {
        return;
    }

    updateMovement(deltaSeconds, localPlayerId_, input);

    if (input.shootPressed) {
        handleShoot(localPlayerId_);
    }

    world_.advanceTick();
}

void GameSession::updateAuthoritative(
    const float deltaSeconds,
    const InputState& hostInput,
    const InputState& clientInput
)
{
    shotFiredThisFrame_ = false;
    damageDealtThisFrame_ = 0;
    updateWeaponCooldown(deltaSeconds);

    if (world_.phase() == domain::MatchPhase::GameOver) {
        if (hostInput.restartPressed || clientInput.restartPressed) {
            restart();
        }
        return;
    }

    if (world_.phase() != domain::MatchPhase::Running) {
        return;
    }

    updateMovement(deltaSeconds, domain::PlayerId::Host, hostInput);
    updateMovement(deltaSeconds, domain::PlayerId::Client, clientInput);

    if (hostInput.shootPressed) {
        handleShoot(domain::PlayerId::Host);
    }
    if (clientInput.shootPressed) {
        handleShoot(domain::PlayerId::Client);
    }

    world_.advanceTick();
}

const domain::World& GameSession::world() const noexcept
{
    return world_;
}

domain::World& GameSession::world() noexcept
{
    return world_;
}

domain::PlayerId GameSession::localPlayerId() const noexcept
{
    return localPlayerId_;
}

bool GameSession::shotFiredThisFrame() const noexcept
{
    return shotFiredThisFrame_;
}

int GameSession::damageDealtThisFrame() const noexcept
{
    return damageDealtThisFrame_;
}

void GameSession::updateMovement(const float deltaSeconds, const domain::PlayerId playerId, const InputState& input)
{
    domain::PlayerState& player = world_.player(playerId);
    if (!player.alive) {
        return;
    }

    float turnAxis = 0.0F;
    if (input.turnLeft) {
        turnAxis -= 1.0F;
    }
    if (input.turnRight) {
        turnAxis += 1.0F;
    }
    player.angleRadians = domain::wrapAngle(player.angleRadians + (turnAxis * rotationSpeed_ * deltaSeconds));

    domain::Vec2 movement {};
    const domain::Vec2 forward {std::cos(player.angleRadians), std::sin(player.angleRadians)};
    const domain::Vec2 right {-forward.y, forward.x};

    if (input.moveForward) {
        movement += forward;
    }
    if (input.moveBackward) {
        movement += forward * -1.0F;
    }
    if (input.strafeLeft) {
        movement += right * -1.0F;
    }
    if (input.strafeRight) {
        movement += right;
    }

    movement = domain::normalize(movement);
    const domain::Vec2 desired = player.position + (movement * moveSpeed_ * deltaSeconds);
    player.position = domain::CollisionSystem::resolveMovement(world_.map(), player.position, desired, player.radius);
}

void GameSession::updateWeaponCooldown(const float deltaSeconds)
{
    for (domain::PlayerId id : {domain::PlayerId::Host, domain::PlayerId::Client}) {
        domain::PlayerState& player = world_.player(id);
        player.weapon.cooldownRemaining = std::max(0.0F, player.weapon.cooldownRemaining - deltaSeconds);
    }
}

void GameSession::handleShoot(const domain::PlayerId shooterId)
{
    domain::PlayerState& shooter = world_.player(shooterId);
    if (shooter.weapon.cooldownRemaining > 0.0F || !shooter.alive) {
        return;
    }

    shooter.weapon.cooldownRemaining = shooter.weapon.cooldownSeconds;
    const bool localShot = shooterId == localPlayerId_;
    if (localShot) {
        shotFiredThisFrame_ = true;
    }

    if (const auto result = domain::ShootingSystem::fireHitscan(world_, shooterId)) {
        const int damage = shooter.weapon.damage;
        if (localShot) {
            damageDealtThisFrame_ = damage;
        }
        domain::DamageSystem::applyDamage(world_, result->target, damage);
    }
}

}  // namespace doomlike::application

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

void GameSession::restart()
{
    if (!loadedMapPath_.empty()) {
        world_.setMap(infrastructure::TextMapLoader::loadFromFile(loadedMapPath_));
    }
    world_.resetForNewRound();
    world_.setPhase(domain::MatchPhase::Running);
}

void GameSession::update(const float deltaSeconds, const InputState& input)
{
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

    updateMovement(deltaSeconds, input);

    if (input.shootPressed) {
        handleShoot();
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

void GameSession::updateMovement(const float deltaSeconds, const InputState& input)
{
    domain::PlayerState& player = world_.player(localPlayerId_);
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

void GameSession::handleShoot()
{
    domain::PlayerState& shooter = world_.player(localPlayerId_);
    if (shooter.weapon.cooldownRemaining > 0.0F || !shooter.alive) {
        return;
    }

    shooter.weapon.cooldownRemaining = shooter.weapon.cooldownSeconds;

    if (const auto result = domain::ShootingSystem::fireHitscan(world_, localPlayerId_)) {
        domain::DamageSystem::applyDamage(world_, result->target, shooter.weapon.damage);
    }
}

}  // namespace doomlike::application

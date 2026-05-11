#pragma once

#include <filesystem>

#include "application/InputState.hpp"
#include "domain/World.hpp"

namespace doomlike::application {

class GameSession {
public:
    bool loadMap(const std::filesystem::path& mapPath);
    void startOfflineDemo();
    void startWaitingForPlayers();
    void restart();
    void setLocalPlayerId(domain::PlayerId playerId) noexcept;
    void update(float deltaSeconds, const InputState& input);
    void updateAuthoritative(float deltaSeconds, const InputState& hostInput, const InputState& clientInput);

    [[nodiscard]] const domain::World& world() const noexcept;
    [[nodiscard]] domain::World& world() noexcept;
    [[nodiscard]] domain::PlayerId localPlayerId() const noexcept;
    [[nodiscard]] bool shotFiredThisFrame() const noexcept;
    [[nodiscard]] int damageDealtThisFrame() const noexcept;

private:
    void updateMovement(float deltaSeconds, domain::PlayerId playerId, const InputState& input);
    void updateWeaponCooldown(float deltaSeconds);
    void handleShoot(domain::PlayerId shooterId);

    domain::World world_ {};
    std::filesystem::path loadedMapPath_ {};
    domain::PlayerId localPlayerId_ {domain::PlayerId::Host};
    float moveSpeed_ {2.8F};
    float rotationSpeed_ {1.9F};
    bool shotFiredThisFrame_ {false};
    int damageDealtThisFrame_ {0};
};

}  // namespace doomlike::application

#pragma once

#include <filesystem>

#include "application/InputState.hpp"
#include "domain/World.hpp"

namespace doomlike::application {

class GameSession {
public:
    bool loadMap(const std::filesystem::path& mapPath);
    void startOfflineDemo();
    void restart();
    void update(float deltaSeconds, const InputState& input);

    [[nodiscard]] const domain::World& world() const noexcept;
    [[nodiscard]] domain::World& world() noexcept;
    [[nodiscard]] domain::PlayerId localPlayerId() const noexcept;

private:
    void updateMovement(float deltaSeconds, const InputState& input);
    void updateWeaponCooldown(float deltaSeconds);
    void handleShoot();

    domain::World world_ {};
    std::filesystem::path loadedMapPath_ {};
    domain::PlayerId localPlayerId_ {domain::PlayerId::Host};
    float moveSpeed_ {2.8F};
    float rotationSpeed_ {1.9F};
};

}  // namespace doomlike::application


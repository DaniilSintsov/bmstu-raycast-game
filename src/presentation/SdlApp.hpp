#pragma once

#include <array>
#include <string_view>
#include <vector>

#include <SDL3/SDL.h>

#include "application/InputState.hpp"
#include "domain/World.hpp"
#include "presentation/Raycaster.hpp"

namespace doomlike::presentation {

class SdlApp {
public:
    SdlApp() = default;
    ~SdlApp();

    bool initialize(std::string_view title, int width = 1280, int height = 720);
    void shutdown();

    [[nodiscard]] application::InputState pollInput();
    void triggerBulletTrail();
    void triggerDamageNumber(int amount);
    void render(const domain::World& world, domain::PlayerId viewerId);

    [[nodiscard]] bool isRunning() const noexcept;

private:
    void updateMovementKeys(SDL_Scancode scancode, bool pressed);
    [[nodiscard]] bool loadEnemyTexture();
    [[nodiscard]] bool loadWeaponTexture();
    void drawBackground();
    void drawWalls(const std::vector<RayHit>& columns);
    void drawSprites(
        const domain::World& world,
        domain::PlayerId viewerId,
        const std::vector<RayHit>& zBuffer
    );
    void drawBulletTrail();
    void drawDamageNumber(
        const domain::World& world,
        domain::PlayerId viewerId,
        const std::vector<RayHit>& zBuffer
    );
    void drawDamageDigit(int digit, float x, float y, float scale, Uint8 alpha);
    void drawWeapon();
    void drawHud(const domain::World& world, domain::PlayerId viewerId);
    [[nodiscard]] SDL_FRect weaponDestinationRect() const noexcept;
    [[nodiscard]] SDL_FPoint weaponMuzzlePoint() const noexcept;

    SDL_Window* window_ {nullptr};
    SDL_Renderer* renderer_ {nullptr};
    SDL_Texture* enemyTexture_ {nullptr};
    SDL_Texture* weaponTexture_ {nullptr};
    Raycaster raycaster_ {};
    bool running_ {false};
    bool sdlInitialized_ {false};
    int logicalWidth_ {1280};
    int logicalHeight_ {720};
    float enemyTextureWidth_ {0.0F};
    float enemyTextureHeight_ {0.0F};
    float weaponTextureWidth_ {0.0F};
    float weaponTextureHeight_ {0.0F};
    Uint64 bulletTrailStartTicks_ {0};
    Uint64 bulletTrailDurationTicks_ {180};
    Uint64 damageNumberStartTicks_ {0};
    Uint64 damageNumberDurationTicks_ {760};
    int damageNumberAmount_ {0};

    bool moveForwardHeld_ {false};
    bool moveBackwardHeld_ {false};
    bool strafeLeftHeld_ {false};
    bool strafeRightHeld_ {false};
    bool turnLeftHeld_ {false};
    bool turnRightHeld_ {false};
};

}  // namespace doomlike::presentation

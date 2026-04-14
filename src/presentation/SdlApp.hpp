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
    void render(const domain::World& world, domain::PlayerId viewerId);

    [[nodiscard]] bool isRunning() const noexcept;

private:
    void updateMovementKeys(SDL_Scancode scancode, bool pressed);
    void drawBackground();
    void drawWalls(const std::vector<RayHit>& columns);
    void drawSprites(
        const domain::World& world,
        domain::PlayerId viewerId,
        const std::vector<RayHit>& zBuffer
    );
    void drawHud(const domain::World& world, domain::PlayerId viewerId);

    SDL_Window* window_ {nullptr};
    SDL_Renderer* renderer_ {nullptr};
    Raycaster raycaster_ {};
    bool running_ {false};
    bool sdlInitialized_ {false};
    int logicalWidth_ {1280};
    int logicalHeight_ {720};

    bool moveForwardHeld_ {false};
    bool moveBackwardHeld_ {false};
    bool strafeLeftHeld_ {false};
    bool strafeRightHeld_ {false};
    bool turnLeftHeld_ {false};
    bool turnRightHeld_ {false};
};

}  // namespace doomlike::presentation

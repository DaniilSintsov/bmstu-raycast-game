#include "presentation/SdlApp.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace doomlike::presentation {

namespace {

[[nodiscard]] Uint8 toChannel(const float value)
{
    return static_cast<Uint8>(std::clamp(value, 0.0F, 255.0F));
}

void setColor(SDL_Renderer* renderer, const Uint8 r, const Uint8 g, const Uint8 b, const Uint8 a = 255)
{
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

}  // namespace

SdlApp::~SdlApp()
{
    shutdown();
}

bool SdlApp::initialize(std::string_view title, const int width, const int height)
{
    logicalWidth_ = width;
    logicalHeight_ = height;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }
    sdlInitialized_ = true;

    if (!SDL_CreateWindowAndRenderer(
            title.data(),
            logicalWidth_,
            logicalHeight_,
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY,
            &window_,
            &renderer_
        )) {
        SDL_Log("SDL_CreateWindowAndRenderer failed: %s", SDL_GetError());
        shutdown();
        return false;
    }

    SDL_SetRenderLogicalPresentation(renderer_, logicalWidth_, logicalHeight_, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    running_ = true;
    return true;
}

void SdlApp::shutdown()
{
    if (renderer_ != nullptr) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }

    if (window_ != nullptr) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    if (sdlInitialized_) {
        running_ = false;
        sdlInitialized_ = false;
        SDL_Quit();
    }
}

application::InputState SdlApp::pollInput()
{
    application::InputState input {};
    SDL_Event event {};

    while (SDL_PollEvent(&event)) {
        if (renderer_ != nullptr) {
            SDL_ConvertEventToRenderCoordinates(renderer_, &event);
        }

        switch (event.type) {
            case SDL_EVENT_QUIT:
                running_ = false;
                input.quitRequested = true;
                break;

            case SDL_EVENT_KEY_DOWN:
                if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
                    running_ = false;
                    input.quitRequested = true;
                    break;
                }

                updateMovementKeys(event.key.scancode, true);

                if (!event.key.repeat) {
                    if (event.key.scancode == SDL_SCANCODE_SPACE) {
                        input.shootPressed = true;
                    }
                    if (event.key.scancode == SDL_SCANCODE_R) {
                        input.restartPressed = true;
                    }
                }
                break;

            case SDL_EVENT_KEY_UP:
                updateMovementKeys(event.key.scancode, false);
                break;

            default:
                break;
        }
    }

    input.moveForward = moveForwardHeld_;
    input.moveBackward = moveBackwardHeld_;
    input.strafeLeft = strafeLeftHeld_;
    input.strafeRight = strafeRightHeld_;
    input.turnLeft = turnLeftHeld_;
    input.turnRight = turnRightHeld_;

    return input;
}

void SdlApp::render(const domain::World& world, const domain::PlayerId viewerId)
{
    if (renderer_ == nullptr) {
        return;
    }

    drawBackground();

    const domain::PlayerState& viewer = world.player(viewerId);
    const std::vector<RayHit> columns =
        raycaster_.castColumns(world.map(), viewer.position, viewer.angleRadians, logicalWidth_);

    drawWalls(columns);
    drawSprites(world, viewerId, columns);
    drawHud(world, viewerId);

    SDL_RenderPresent(renderer_);
}

bool SdlApp::isRunning() const noexcept
{
    return running_;
}

void SdlApp::updateMovementKeys(const SDL_Scancode scancode, const bool pressed)
{
    switch (scancode) {
        case SDL_SCANCODE_W:
            moveForwardHeld_ = pressed;
            break;
        case SDL_SCANCODE_S:
            moveBackwardHeld_ = pressed;
            break;
        case SDL_SCANCODE_A:
            strafeLeftHeld_ = pressed;
            break;
        case SDL_SCANCODE_D:
            strafeRightHeld_ = pressed;
            break;
        case SDL_SCANCODE_LEFT:
            turnLeftHeld_ = pressed;
            break;
        case SDL_SCANCODE_RIGHT:
            turnRightHeld_ = pressed;
            break;
        default:
            break;
    }
}

void SdlApp::drawBackground()
{
    setColor(renderer_, 0, 0, 0);
    SDL_RenderClear(renderer_);

    const int horizon = logicalHeight_ / 2;

    for (int y = 0; y < horizon; ++y) {
        const float t = static_cast<float>(y) / static_cast<float>(horizon);
        setColor(
            renderer_,
            toChannel(25.0F + (40.0F * t)),
            toChannel(40.0F + (90.0F * t)),
            toChannel(90.0F + (120.0F * t))
        );
        SDL_RenderLine(renderer_, 0.0F, static_cast<float>(y), static_cast<float>(logicalWidth_), static_cast<float>(y));
    }

    for (int y = horizon; y < logicalHeight_; ++y) {
        const float t = static_cast<float>(y - horizon) / static_cast<float>(horizon);
        setColor(
            renderer_,
            toChannel(55.0F + (30.0F * t)),
            toChannel(45.0F + (20.0F * t)),
            toChannel(30.0F + (10.0F * t))
        );
        SDL_RenderLine(renderer_, 0.0F, static_cast<float>(y), static_cast<float>(logicalWidth_), static_cast<float>(y));
    }
}

void SdlApp::drawWalls(const std::vector<RayHit>& columns)
{
    for (std::size_t x = 0; x < columns.size(); ++x) {
        const RayHit& hit = columns[x];
        if (!hit.hit) {
            continue;
        }

        const float lineHeight = static_cast<float>(logicalHeight_) / std::max(hit.distance, 0.0001F);
        const float startY = std::max(0.0F, (static_cast<float>(logicalHeight_) * 0.5F) - (lineHeight * 0.5F));
        const float endY = std::min(static_cast<float>(logicalHeight_ - 1), startY + lineHeight);

        const float brightness = std::clamp(1.0F - (hit.distance / 20.0F), 0.25F, 1.0F);
        const float sideMultiplier = hit.side == 0 ? 1.0F : 0.75F;

        setColor(
            renderer_,
            toChannel(125.0F * brightness * sideMultiplier),
            toChannel(125.0F * brightness * sideMultiplier),
            toChannel(135.0F * brightness * sideMultiplier)
        );

        const float screenX = static_cast<float>(x);
        SDL_RenderLine(renderer_, screenX, startY, screenX, endY);
    }
}

void SdlApp::drawSprites(
    const domain::World& world,
    const domain::PlayerId viewerId,
    const std::vector<RayHit>& zBuffer
)
{
    const domain::PlayerState& viewer = world.player(viewerId);
    const domain::PlayerState& enemy = world.enemyOf(viewerId);
    if (!enemy.alive) {
        return;
    }

    const domain::Vec2 delta = enemy.position - viewer.position;
    const float distance = domain::length(delta);
    if (distance <= 0.05F) {
        return;
    }

    const float angleToEnemy = domain::angleBetween(viewer.position, enemy.position);
    const float relativeAngle = domain::wrapAngle(angleToEnemy - viewer.angleRadians);
    const float halfFov = raycaster_.fovRadians() * 0.5F;
    if (std::abs(relativeAngle) > halfFov + domain::degToRad(8.0F)) {
        return;
    }

    const float normalized = (relativeAngle / raycaster_.fovRadians()) + 0.5F;
    const float screenCenterX = normalized * static_cast<float>(logicalWidth_);
    const float spriteHeight = static_cast<float>(logicalHeight_) / distance;
    const float spriteWidth = spriteHeight * 0.60F;
    const int xStart = std::max(0, static_cast<int>(screenCenterX - (spriteWidth * 0.5F)));
    const int xEnd = std::min(logicalWidth_ - 1, static_cast<int>(screenCenterX + (spriteWidth * 0.5F)));
    const float yStart = std::max(0.0F, (static_cast<float>(logicalHeight_) * 0.5F) - (spriteHeight * 0.5F));
    const float yEnd = std::min(static_cast<float>(logicalHeight_ - 1), yStart + spriteHeight);

    for (int x = xStart; x <= xEnd; ++x) {
        const std::size_t bufferIndex = static_cast<std::size_t>(x);
        if (bufferIndex >= zBuffer.size()) {
            continue;
        }

        if (distance >= zBuffer[bufferIndex].distance) {
            continue;
        }

        setColor(renderer_, 220, 60, 60);
        SDL_RenderLine(renderer_, static_cast<float>(x), yStart, static_cast<float>(x), yEnd);
    }
}

void SdlApp::drawHud(const domain::World& world, const domain::PlayerId viewerId)
{
    const domain::PlayerState& localPlayer = world.player(viewerId);
    const domain::PlayerState& enemy = world.enemyOf(viewerId);

    setColor(renderer_, 255, 255, 255);
    SDL_RenderLine(
        renderer_,
        static_cast<float>(logicalWidth_ / 2 - 10),
        static_cast<float>(logicalHeight_ / 2),
        static_cast<float>(logicalWidth_ / 2 + 10),
        static_cast<float>(logicalHeight_ / 2)
    );
    SDL_RenderLine(
        renderer_,
        static_cast<float>(logicalWidth_ / 2),
        static_cast<float>(logicalHeight_ / 2 - 10),
        static_cast<float>(logicalWidth_ / 2),
        static_cast<float>(logicalHeight_ / 2 + 10)
    );

    SDL_RenderDebugTextFormat(renderer_, 16.0F, 16.0F, "HP: %d", localPlayer.hp);
    SDL_RenderDebugTextFormat(renderer_, 16.0F, 28.0F, "Enemy HP: %d", enemy.hp);
    SDL_RenderDebugText(renderer_, 16.0F, 40.0F, "WASD move, arrows turn, Space shoot, R restart, Esc quit");

    switch (world.phase()) {
        case domain::MatchPhase::Running:
            SDL_RenderDebugText(renderer_, 16.0F, 52.0F, "Mode: offline combat demo");
            break;
        case domain::MatchPhase::GameOver:
            if (world.winner().has_value() && world.winner().value() == viewerId) {
                SDL_RenderDebugText(renderer_, 16.0F, 52.0F, "You win. Press R to restart or Esc to exit.");
            } else {
                SDL_RenderDebugText(renderer_, 16.0F, 52.0F, "You lose. Press R to restart or Esc to exit.");
            }
            break;
        case domain::MatchPhase::OpponentDisconnected:
            SDL_RenderDebugText(renderer_, 16.0F, 52.0F, "Opponent disconnected. Press R to restart.");
            break;
        default:
            SDL_RenderDebugText(renderer_, 16.0F, 52.0F, "Waiting for players...");
            break;
    }
}

}  // namespace doomlike::presentation

#include "presentation/SdlApp.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace doomlike::presentation {

namespace {

[[nodiscard]] float interpolate(const float from, const float to, const float t) noexcept
{
    return from + ((to - from) * t);
}

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
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    if (!loadEnemyTexture()) {
        SDL_Log("Enemy sprite texture is unavailable; using fallback billboard");
    }
    if (!loadWeaponTexture()) {
        SDL_Log("Weapon sprite texture is unavailable; using fallback weapon");
    }

    running_ = true;
    return true;
}

void SdlApp::shutdown()
{
    if (enemyTexture_ != nullptr) {
        SDL_DestroyTexture(enemyTexture_);
        enemyTexture_ = nullptr;
        enemyTextureWidth_ = 0.0F;
        enemyTextureHeight_ = 0.0F;
    }

    if (weaponTexture_ != nullptr) {
        SDL_DestroyTexture(weaponTexture_);
        weaponTexture_ = nullptr;
        weaponTextureWidth_ = 0.0F;
        weaponTextureHeight_ = 0.0F;
    }

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
    drawDamageNumber(world, viewerId, columns);
    drawBulletTrail();
    drawWeapon();
    drawHud(world, viewerId);

    SDL_RenderPresent(renderer_);
}

bool SdlApp::isRunning() const noexcept
{
    return running_;
}

void SdlApp::setHudModeLabel(std::string label)
{
    hudModeLabel_ = std::move(label);
}

std::optional<application::LaunchOptions> SdlApp::runStartupMenu()
{
    std::string hostIp {};
    bool editingIp = false;
    bool textInputActive = false;

    const auto setTextInput = [&](const bool active) {
        if (window_ == nullptr || textInputActive == active) {
            return;
        }

        if (active) {
            SDL_StartTextInput(window_);
        } else {
            SDL_StopTextInput(window_);
        }
        textInputActive = active;
    };

    while (running_) {
        SDL_Event event {};
        while (SDL_PollEvent(&event)) {
            if (renderer_ != nullptr) {
                SDL_ConvertEventToRenderCoordinates(renderer_, &event);
            }

            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running_ = false;
                    setTextInput(false);
                    return std::nullopt;

                case SDL_EVENT_TEXT_INPUT:
                    if (editingIp && event.text.text != nullptr) {
                        const std::string_view text(event.text.text);
                        for (const char ch : text) {
                            const bool allowed =
                                (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') ||
                                (ch >= 'A' && ch <= 'Z') || ch == '.' || ch == '-' || ch == ':';
                            if (allowed && hostIp.size() < 63) {
                                hostIp.push_back(ch);
                            }
                        }
                    }
                    break;

                case SDL_EVENT_KEY_DOWN:
                    if (event.key.repeat) {
                        break;
                    }

                    if (editingIp) {
                        if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
                            editingIp = false;
                            setTextInput(false);
                            break;
                        }
                        if (event.key.scancode == SDL_SCANCODE_BACKSPACE && !hostIp.empty()) {
                            hostIp.pop_back();
                            break;
                        }
                        if ((event.key.scancode == SDL_SCANCODE_RETURN ||
                             event.key.scancode == SDL_SCANCODE_KP_ENTER) &&
                            !hostIp.empty()) {
                            setTextInput(false);
                            return application::LaunchOptions {
                                .mode = application::GameMode::MultiplayerClient,
                                .hostIp = hostIp,
                                .port = application::kDefaultNetworkPort,
                            };
                        }
                        break;
                    }

                    if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
                        running_ = false;
                        return std::nullopt;
                    }
                    if (event.key.scancode == SDL_SCANCODE_1) {
                        return application::LaunchOptions {.mode = application::GameMode::Test};
                    }
                    if (event.key.scancode == SDL_SCANCODE_2) {
                        return application::LaunchOptions {
                            .mode = application::GameMode::MultiplayerHost,
                            .port = application::kDefaultNetworkPort,
                        };
                    }
                    if (event.key.scancode == SDL_SCANCODE_3) {
                        editingIp = true;
                        setTextInput(true);
                    }
                    break;

                default:
                    break;
            }
        }

        drawStartupMenu(editingIp, hostIp);
        SDL_Delay(16);
    }

    setTextInput(false);
    return std::nullopt;
}

void SdlApp::triggerBulletTrail()
{
    bulletTrailStartTicks_ = SDL_GetTicks();
}

void SdlApp::triggerDamageNumber(const int amount)
{
    damageNumberAmount_ = amount;
    damageNumberStartTicks_ = SDL_GetTicks();
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

void SdlApp::drawStartupMenu(const bool editingIp, const std::string_view hostIp)
{
    if (renderer_ == nullptr) {
        return;
    }

    drawBackground();

    setColor(renderer_, 255, 255, 255);
    SDL_RenderDebugText(renderer_, 96.0F, 96.0F, "BAUMAN DOOMLIKE");
    SDL_RenderDebugText(renderer_, 96.0F, 132.0F, "Select start mode:");
    SDL_RenderDebugText(renderer_, 112.0F, 172.0F, "1  Test mode");
    SDL_RenderDebugText(renderer_, 112.0F, 196.0F, "2  Multiplayer: host LAN game");
    SDL_RenderDebugText(renderer_, 112.0F, 220.0F, "3  Multiplayer: connect by host IP");

    if (editingIp) {
        SDL_RenderDebugTextFormat(renderer_, 112.0F, 276.0F, "Host IP: %s_", std::string(hostIp).c_str());
        SDL_RenderDebugText(renderer_, 112.0F, 300.0F, "Enter connects, Backspace edits, Esc returns.");
    } else {
        SDL_RenderDebugTextFormat(
            renderer_,
            112.0F,
            276.0F,
            "LAN port: %u",
            static_cast<unsigned>(application::kDefaultNetworkPort)
        );
        SDL_RenderDebugText(renderer_, 112.0F, 300.0F, "Host: choose 2. Client: choose 3 and type host LAN IP.");
        SDL_RenderDebugText(renderer_, 112.0F, 324.0F, "Esc quits.");
    }

    SDL_RenderPresent(renderer_);
}

bool SdlApp::loadEnemyTexture()
{
    if (renderer_ == nullptr) {
        return false;
    }

    const std::filesystem::path texturePath =
        std::filesystem::path(DOOMLIKE_PROJECT_SOURCE_DIR) / "resources" / "textures" / "enemy.bmp";
    const std::string texturePathString = texturePath.string();

    SDL_Surface* surface = SDL_LoadBMP(texturePathString.c_str());
    if (surface == nullptr) {
        SDL_Log("Failed to load enemy sprite '%s': %s", texturePathString.c_str(), SDL_GetError());
        return false;
    }

    const SDL_PixelFormatDetails* formatDetails = SDL_GetPixelFormatDetails(surface->format);
    if (formatDetails == nullptr) {
        SDL_Log("Failed to inspect enemy sprite pixel format '%s': %s", texturePathString.c_str(), SDL_GetError());
        SDL_DestroySurface(surface);
        return false;
    }

    const Uint32 colorKey = SDL_MapRGB(formatDetails, SDL_GetSurfacePalette(surface), 0, 255, 0);
    if (!SDL_SetSurfaceColorKey(surface, true, colorKey)) {
        SDL_Log("Failed to set enemy sprite color key '%s': %s", texturePathString.c_str(), SDL_GetError());
        SDL_DestroySurface(surface);
        return false;
    }

    enemyTexture_ = SDL_CreateTextureFromSurface(renderer_, surface);
    SDL_DestroySurface(surface);

    if (enemyTexture_ == nullptr) {
        SDL_Log("Failed to create enemy sprite texture '%s': %s", texturePathString.c_str(), SDL_GetError());
        return false;
    }

    SDL_SetTextureScaleMode(enemyTexture_, SDL_SCALEMODE_NEAREST);
    SDL_GetTextureSize(enemyTexture_, &enemyTextureWidth_, &enemyTextureHeight_);
    return true;
}

bool SdlApp::loadWeaponTexture()
{
    if (renderer_ == nullptr) {
        return false;
    }

    const std::filesystem::path texturePath =
        std::filesystem::path(DOOMLIKE_PROJECT_SOURCE_DIR) / "resources" / "textures" / "weapon.bmp";
    const std::string texturePathString = texturePath.string();

    SDL_Surface* surface = SDL_LoadBMP(texturePathString.c_str());
    if (surface == nullptr) {
        SDL_Log("Failed to load weapon sprite '%s': %s", texturePathString.c_str(), SDL_GetError());
        return false;
    }

    const SDL_PixelFormatDetails* formatDetails = SDL_GetPixelFormatDetails(surface->format);
    if (formatDetails == nullptr) {
        SDL_Log("Failed to inspect weapon sprite pixel format '%s': %s", texturePathString.c_str(), SDL_GetError());
        SDL_DestroySurface(surface);
        return false;
    }

    const Uint32 colorKey = SDL_MapRGB(formatDetails, SDL_GetSurfacePalette(surface), 0, 255, 0);
    if (!SDL_SetSurfaceColorKey(surface, true, colorKey)) {
        SDL_Log("Failed to set weapon sprite color key '%s': %s", texturePathString.c_str(), SDL_GetError());
        SDL_DestroySurface(surface);
        return false;
    }

    weaponTexture_ = SDL_CreateTextureFromSurface(renderer_, surface);
    SDL_DestroySurface(surface);

    if (weaponTexture_ == nullptr) {
        SDL_Log("Failed to create weapon sprite texture '%s': %s", texturePathString.c_str(), SDL_GetError());
        return false;
    }

    SDL_SetTextureScaleMode(weaponTexture_, SDL_SCALEMODE_NEAREST);
    SDL_GetTextureSize(weaponTexture_, &weaponTextureWidth_, &weaponTextureHeight_);
    return true;
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
    const float spriteLeft = screenCenterX - (spriteWidth * 0.5F);

    for (int x = xStart; x <= xEnd; ++x) {
        const std::size_t bufferIndex = static_cast<std::size_t>(x);
        if (bufferIndex >= zBuffer.size()) {
            continue;
        }

        if (distance >= zBuffer[bufferIndex].distance) {
            continue;
        }

        if (enemyTexture_ == nullptr || enemyTextureWidth_ <= 0.0F || enemyTextureHeight_ <= 0.0F) {
            setColor(renderer_, 220, 60, 60);
            SDL_RenderLine(renderer_, static_cast<float>(x), yStart, static_cast<float>(x), yEnd);
            continue;
        }

        const float sourceX =
            std::clamp(((static_cast<float>(x) + 0.5F - spriteLeft) / spriteWidth) * enemyTextureWidth_, 0.0F, enemyTextureWidth_ - 1.0F);
        const SDL_FRect source {sourceX, 0.0F, 1.0F, enemyTextureHeight_};
        const SDL_FRect destination {static_cast<float>(x), yStart, 1.0F, yEnd - yStart};
        SDL_RenderTexture(renderer_, enemyTexture_, &source, &destination);
    }
}

void SdlApp::drawBulletTrail()
{
    if (bulletTrailStartTicks_ == 0) {
        return;
    }

    const Uint64 now = SDL_GetTicks();
    const Uint64 elapsedTicks = now > bulletTrailStartTicks_ ? now - bulletTrailStartTicks_ : 0;
    if (elapsedTicks >= bulletTrailDurationTicks_) {
        bulletTrailStartTicks_ = 0;
        return;
    }

    const float progress = static_cast<float>(elapsedTicks) / static_cast<float>(bulletTrailDurationTicks_);
    const float easedProgress = 1.0F - ((1.0F - progress) * (1.0F - progress));

    const SDL_FPoint muzzle = weaponMuzzlePoint();
    const float startX = muzzle.x;
    const float startY = muzzle.y;
    const float endX = static_cast<float>(logicalWidth_) * 0.5F;
    const float endY = static_cast<float>(logicalHeight_) * 0.5F;

    const float headX = interpolate(startX, endX, easedProgress);
    const float headY = interpolate(startY, endY, easedProgress);
    const float tailProgress = std::max(0.0F, easedProgress - 0.20F);
    const float tailX = interpolate(startX, endX, tailProgress);
    const float tailY = interpolate(startY, endY, tailProgress);
    const float fade = 1.0F - progress;

    setColor(renderer_, 255, toChannel(230.0F * fade), toChannel(80.0F * fade));
    SDL_RenderLine(renderer_, tailX, tailY, headX, headY);
    SDL_RenderLine(renderer_, tailX + 1.0F, tailY, headX + 1.0F, headY);
    SDL_RenderLine(renderer_, tailX - 1.0F, tailY, headX - 1.0F, headY);

    setColor(renderer_, 255, 245, 180);
    const float bulletSize = 6.0F + (5.0F * fade);
    const SDL_FRect bullet {
        headX - (bulletSize * 0.5F),
        headY - (bulletSize * 0.5F),
        bulletSize,
        bulletSize,
    };
    SDL_RenderFillRect(renderer_, &bullet);
}

void SdlApp::drawDamageNumber(
    const domain::World& world,
    const domain::PlayerId viewerId,
    const std::vector<RayHit>& zBuffer
)
{
    if (damageNumberStartTicks_ == 0 || damageNumberAmount_ <= 0) {
        return;
    }

    const Uint64 now = SDL_GetTicks();
    const Uint64 elapsedTicks = now > damageNumberStartTicks_ ? now - damageNumberStartTicks_ : 0;
    if (elapsedTicks >= damageNumberDurationTicks_) {
        damageNumberStartTicks_ = 0;
        damageNumberAmount_ = 0;
        return;
    }

    const domain::PlayerState& viewer = world.player(viewerId);
    const domain::PlayerState& enemy = world.enemyOf(viewerId);
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
    const int centerColumn = static_cast<int>(std::clamp(screenCenterX, 0.0F, static_cast<float>(logicalWidth_ - 1)));
    const std::size_t bufferIndex = static_cast<std::size_t>(centerColumn);
    if (bufferIndex >= zBuffer.size() || distance >= zBuffer[bufferIndex].distance) {
        return;
    }

    const float spriteHeight = static_cast<float>(logicalHeight_) / distance;
    const float yStart = std::max(0.0F, (static_cast<float>(logicalHeight_) * 0.5F) - (spriteHeight * 0.5F));
    const float progress = static_cast<float>(elapsedTicks) / static_cast<float>(damageNumberDurationTicks_);
    const float fade = 1.0F - progress;
    const Uint8 alpha = toChannel(255.0F * fade);
    const float scale = 5.0F + (1.5F * (1.0F - fade));
    const std::string text = std::to_string(damageNumberAmount_);
    const float digitWidth = 5.0F * scale;
    const float spacing = 2.0F * scale;
    const float totalWidth =
        (static_cast<float>(text.size()) * digitWidth) + (static_cast<float>(text.size() - 1) * spacing);
    const float xStart = screenCenterX - (totalWidth * 0.5F);
    const float y = yStart - 34.0F - (progress * 56.0F);

    for (std::size_t index = 0; index < text.size(); ++index) {
        const int digit = text[index] - '0';
        if (digit < 0 || digit > 9) {
            continue;
        }
        drawDamageDigit(digit, xStart + (static_cast<float>(index) * (digitWidth + spacing)), y, scale, alpha);
    }
}

void SdlApp::drawDamageDigit(const int digit, const float x, const float y, const float scale, const Uint8 alpha)
{
    static constexpr std::array<std::array<bool, 7>, 10> kSegments {{
        {{true, true, true, true, true, true, false}},
        {{false, true, true, false, false, false, false}},
        {{true, true, false, true, true, false, true}},
        {{true, true, true, true, false, false, true}},
        {{false, true, true, false, false, true, true}},
        {{true, false, true, true, false, true, true}},
        {{true, false, true, true, true, true, true}},
        {{true, true, true, false, false, false, false}},
        {{true, true, true, true, true, true, true}},
        {{true, true, true, true, false, true, true}},
    }};

    const float thick = scale;
    const std::array<SDL_FRect, 7> rects {{
        SDL_FRect {x + thick, y, thick * 3.0F, thick},
        SDL_FRect {x + (thick * 4.0F), y + thick, thick, thick * 3.0F},
        SDL_FRect {x + (thick * 4.0F), y + (thick * 5.0F), thick, thick * 3.0F},
        SDL_FRect {x + thick, y + (thick * 8.0F), thick * 3.0F, thick},
        SDL_FRect {x, y + (thick * 5.0F), thick, thick * 3.0F},
        SDL_FRect {x, y + thick, thick, thick * 3.0F},
        SDL_FRect {x + thick, y + (thick * 4.0F), thick * 3.0F, thick},
    }};

    setColor(renderer_, 0, 0, 0, toChannel(static_cast<float>(alpha) * 0.65F));
    for (std::size_t index = 0; index < rects.size(); ++index) {
        if (!kSegments[static_cast<std::size_t>(digit)][index]) {
            continue;
        }
        SDL_FRect shadow = rects[index];
        shadow.x += scale * 0.60F;
        shadow.y += scale * 0.60F;
        SDL_RenderFillRect(renderer_, &shadow);
    }

    setColor(renderer_, 255, 215, 55, alpha);
    for (std::size_t index = 0; index < rects.size(); ++index) {
        if (kSegments[static_cast<std::size_t>(digit)][index]) {
            SDL_RenderFillRect(renderer_, &rects[index]);
        }
    }
}

void SdlApp::drawWeapon()
{
    const SDL_FRect destination = weaponDestinationRect();

    if (weaponTexture_ != nullptr && weaponTextureWidth_ > 0.0F && weaponTextureHeight_ > 0.0F) {
        const SDL_FRect source {0.0F, 0.0F, weaponTextureWidth_, weaponTextureHeight_};
        SDL_RenderTexture(renderer_, weaponTexture_, &source, &destination);
        return;
    }

    setColor(renderer_, 42, 43, 48);
    SDL_RenderFillRect(renderer_, &destination);

    setColor(renderer_, 125, 126, 132);
    const SDL_FRect barrel {
        destination.x + (destination.w * 0.44F),
        destination.y + (destination.h * 0.08F),
        destination.w * 0.12F,
        destination.h * 0.52F,
    };
    SDL_RenderFillRect(renderer_, &barrel);

    setColor(renderer_, 220, 70, 35);
    const SDL_FPoint muzzle = weaponMuzzlePoint();
    const SDL_FRect muzzleFlash {muzzle.x - 5.0F, muzzle.y - 5.0F, 10.0F, 10.0F};
    SDL_RenderFillRect(renderer_, &muzzleFlash);
}

SDL_FRect SdlApp::weaponDestinationRect() const noexcept
{
    const float width = static_cast<float>(logicalWidth_) * 0.32F;
    const float aspect = weaponTextureWidth_ > 0.0F && weaponTextureHeight_ > 0.0F
                             ? weaponTextureHeight_ / weaponTextureWidth_
                             : 0.78F;
    const float height = width * aspect;
    constexpr float kMuzzleSourceX = 0.515F;

    return SDL_FRect {
        (static_cast<float>(logicalWidth_) * 0.5F) - (width * kMuzzleSourceX),
        static_cast<float>(logicalHeight_) - height + 118.0F,
        width,
        height,
    };
}

SDL_FPoint SdlApp::weaponMuzzlePoint() const noexcept
{
    const SDL_FRect destination = weaponDestinationRect();
    return SDL_FPoint {
        destination.x + (destination.w * 0.515F),
        destination.y + (destination.h * 0.080F),
    };
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
            SDL_RenderDebugTextFormat(renderer_, 16.0F, 52.0F, "Mode: %s", hudModeLabel_.c_str());
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

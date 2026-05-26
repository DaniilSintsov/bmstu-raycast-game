#include "application/GameLoop.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>

#include "application/GameSession.hpp"
#include "application/LaunchOptions.hpp"
#include "network/EnetClient.hpp"
#include "network/EnetHost.hpp"
#include "network/PacketSerializer.hpp"
#include "network/Protocol.hpp"
#include "presentation/SdlApp.hpp"

namespace doomlike::application {

namespace {

inline constexpr float kStateBroadcastInterval = 1.0F / 30.0F;

[[nodiscard]] std::uint8_t playerIdToByte(const domain::PlayerId id) noexcept
{
    return static_cast<std::uint8_t>(id);
}

[[nodiscard]] std::optional<domain::PlayerId> playerIdFromByte(const std::uint8_t value) noexcept
{
    if (value == playerIdToByte(domain::PlayerId::Host)) {
        return domain::PlayerId::Host;
    }
    if (value == playerIdToByte(domain::PlayerId::Client)) {
        return domain::PlayerId::Client;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<domain::MatchPhase> phaseFromByte(const std::uint8_t value) noexcept
{
    switch (static_cast<domain::MatchPhase>(value)) {
        case domain::MatchPhase::Menu:
        case domain::MatchPhase::WaitingForPlayers:
        case domain::MatchPhase::Running:
        case domain::MatchPhase::GameOver:
        case domain::MatchPhase::OpponentDisconnected:
            return static_cast<domain::MatchPhase>(value);
    }
    return std::nullopt;
}

[[nodiscard]] bool hasInputFlag(const std::uint8_t mask, const network::InputFlags flag) noexcept
{
    return (mask & network::toMask(flag)) != 0;
}

[[nodiscard]] std::uint8_t inputToMask(const InputState& input) noexcept
{
    std::uint8_t mask = 0;
    if (input.moveForward) {
        mask |= network::toMask(network::InputFlags::MoveForward);
    }
    if (input.moveBackward) {
        mask |= network::toMask(network::InputFlags::MoveBackward);
    }
    if (input.strafeLeft) {
        mask |= network::toMask(network::InputFlags::StrafeLeft);
    }
    if (input.strafeRight) {
        mask |= network::toMask(network::InputFlags::StrafeRight);
    }
    if (input.turnLeft) {
        mask |= network::toMask(network::InputFlags::TurnLeft);
    }
    if (input.turnRight) {
        mask |= network::toMask(network::InputFlags::TurnRight);
    }
    if (input.shootPressed) {
        mask |= network::toMask(network::InputFlags::Shoot);
    }
    if (input.restartPressed) {
        mask |= network::toMask(network::InputFlags::Restart);
    }
    return mask;
}

[[nodiscard]] InputState inputFromMask(const std::uint8_t mask) noexcept
{
    InputState input {};
    input.moveForward = hasInputFlag(mask, network::InputFlags::MoveForward);
    input.moveBackward = hasInputFlag(mask, network::InputFlags::MoveBackward);
    input.strafeLeft = hasInputFlag(mask, network::InputFlags::StrafeLeft);
    input.strafeRight = hasInputFlag(mask, network::InputFlags::StrafeRight);
    input.turnLeft = hasInputFlag(mask, network::InputFlags::TurnLeft);
    input.turnRight = hasInputFlag(mask, network::InputFlags::TurnRight);
    input.shootPressed = hasInputFlag(mask, network::InputFlags::Shoot);
    input.restartPressed = hasInputFlag(mask, network::InputFlags::Restart);
    return input;
}

[[nodiscard]] network::InputCommandMessage makeInputCommand(
    const domain::PlayerId playerId,
    const InputState& input,
    const std::uint32_t tick
) noexcept
{
    network::InputCommandMessage message {};
    message.header.tick = tick;
    message.playerId = playerIdToByte(playerId);
    message.inputMask = inputToMask(input);
    return message;
}

[[nodiscard]] network::NetworkPlayerSnapshot makeSnapshot(const domain::PlayerState& player) noexcept
{
    return network::NetworkPlayerSnapshot {
        .playerId = playerIdToByte(player.id),
        .x = player.position.x,
        .y = player.position.y,
        .angleRadians = player.angleRadians,
        .hp = static_cast<std::int16_t>(player.hp),
        .alive = static_cast<std::uint8_t>(player.alive ? 1 : 0),
    };
}

[[nodiscard]] network::StateUpdateMessage makeStateUpdate(const domain::World& world) noexcept
{
    network::StateUpdateMessage message {};
    message.header.tick = world.tick();
    message.players[0] = makeSnapshot(world.player(domain::PlayerId::Host));
    message.players[1] = makeSnapshot(world.player(domain::PlayerId::Client));
    message.phase = static_cast<std::uint8_t>(world.phase());
    if (world.winner().has_value()) {
        message.winner = playerIdToByte(world.winner().value());
    }
    return message;
}

[[nodiscard]] network::WelcomeMessage makeWelcome(const domain::World& world, const domain::PlayerId assignedPlayerId)
{
    network::WelcomeMessage message {};
    message.header.tick = world.tick();
    message.assignedPlayerId = playerIdToByte(assignedPlayerId);

    const std::string& mapName = world.map().name();
    std::strncpy(message.mapName, mapName.c_str(), sizeof(message.mapName) - 1);
    message.mapName[sizeof(message.mapName) - 1] = '\0';
    return message;
}

void applyStateUpdate(GameSession& session, const network::StateUpdateMessage& message)
{
    domain::World& world = session.world();
    for (const network::NetworkPlayerSnapshot& snapshot : message.players) {
        const std::optional<domain::PlayerId> playerId = playerIdFromByte(snapshot.playerId);
        if (!playerId.has_value()) {
            continue;
        }

        domain::PlayerState& player = world.player(playerId.value());
        player.position = domain::Vec2 {snapshot.x, snapshot.y};
        player.angleRadians = snapshot.angleRadians;
        player.hp = snapshot.hp;
        player.alive = snapshot.alive != 0;
    }

    if (const std::optional<domain::MatchPhase> phase = phaseFromByte(message.phase)) {
        world.setPhase(phase.value());
    }

    if (message.winner == network::kInvalidWinner) {
        world.setWinner(std::nullopt);
    } else if (const std::optional<domain::PlayerId> winner = playerIdFromByte(message.winner)) {
        world.setWinner(winner);
    }
}

void broadcastState(network::EnetHost& host, const domain::World& world)
{
    (void)host.broadcastBytes(network::PacketSerializer::serialize(makeStateUpdate(world)), false);
}

int runOfflineLoop(presentation::SdlApp& app, GameSession& session)
{
    using clock = std::chrono::steady_clock;
    auto lastTick = clock::now();

    while (app.isRunning()) {
        const auto now = clock::now();
        const float deltaSeconds = std::min(std::chrono::duration<float>(now - lastTick).count(), 0.05F);
        lastTick = now;

        const InputState input = app.pollInput();
        if (input.quitRequested) {
            break;
        }

        session.update(deltaSeconds, input);
        if (session.shotFiredThisFrame()) {
            app.triggerBulletTrail();
        }
        if (session.damageDealtThisFrame() > 0) {
            app.triggerDamageNumber(session.damageDealtThisFrame());
        }
        app.render(session.world(), session.localPlayerId());
    }

    return 0;
}

int runHostLoop(presentation::SdlApp& app, GameSession& session, const LaunchOptions& options)
{
    network::EnetHost host;
    if (!host.start(options.port)) {
        throw std::runtime_error("Failed to start LAN host");
    }

    session.setLocalPlayerId(domain::PlayerId::Host);
    session.startWaitingForPlayers();

    InputState clientInput {};
    float stateBroadcastAccumulator = 0.0F;

    using clock = std::chrono::steady_clock;
    auto lastTick = clock::now();

    while (app.isRunning()) {
        const auto now = clock::now();
        const float deltaSeconds = std::min(std::chrono::duration<float>(now - lastTick).count(), 0.05F);
        lastTick = now;

        const InputState hostInput = app.pollInput();
        if (hostInput.quitRequested) {
            break;
        }

        const bool hadPeer = host.hasPeer();
        host.poll([&clientInput](const network::MessageType type, const std::span<const std::byte> payload) {
            if (type != network::MessageType::InputCommand) {
                return;
            }

            const std::optional<network::InputCommandMessage> message =
                network::PacketSerializer::deserialize<network::InputCommandMessage>(payload);
            if (!message.has_value() || message->playerId != playerIdToByte(domain::PlayerId::Client)) {
                return;
            }

            clientInput = inputFromMask(message->inputMask);
        });

        const bool hasPeer = host.hasPeer();
        if (!hadPeer && hasPeer) {
            (void)host.send(makeWelcome(session.world(), domain::PlayerId::Client), true);
            session.restart();
            broadcastState(host, session.world());
        } else if (hadPeer && !hasPeer) {
            session.world().setPhase(domain::MatchPhase::OpponentDisconnected);
            clientInput = {};
        }

        if (!hasPeer && session.world().phase() == domain::MatchPhase::OpponentDisconnected && hostInput.restartPressed) {
            session.startWaitingForPlayers();
        }

        if (hasPeer) {
            session.updateAuthoritative(deltaSeconds, hostInput, clientInput);
            clientInput.shootPressed = false;
            clientInput.restartPressed = false;

            stateBroadcastAccumulator += deltaSeconds;
            if (stateBroadcastAccumulator >= kStateBroadcastInterval ||
                session.world().phase() == domain::MatchPhase::GameOver) {
                broadcastState(host, session.world());
                stateBroadcastAccumulator = 0.0F;
            }
        }

        if (session.shotFiredThisFrame()) {
            app.triggerBulletTrail();
        }
        if (session.damageDealtThisFrame() > 0) {
            app.triggerDamageNumber(session.damageDealtThisFrame());
        }
        app.render(session.world(), session.localPlayerId());
    }

    return 0;
}

int runClientLoop(presentation::SdlApp& app, GameSession& session, const LaunchOptions& options)
{
    network::EnetClient client;
    if (!client.connect(options.hostIp, options.port)) {
        throw std::runtime_error("Failed to connect to LAN host");
    }

    session.setLocalPlayerId(domain::PlayerId::Client);
    session.startWaitingForPlayers();
    (void)client.send(network::ConnectMessage {}, true);

    using clock = std::chrono::steady_clock;
    auto lastTick = clock::now();

    while (app.isRunning()) {
        const auto now = clock::now();
        const float deltaSeconds = std::min(std::chrono::duration<float>(now - lastTick).count(), 0.05F);
        lastTick = now;
        (void)deltaSeconds;

        const InputState input = app.pollInput();
        if (input.quitRequested) {
            break;
        }

        const bool wasConnected = client.isConnected();
        if (wasConnected) {
            (void)client.send(
                makeInputCommand(session.localPlayerId(), input, session.world().tick()),
                input.shootPressed || input.restartPressed
            );
        }

        const int previousEnemyHp = session.world().enemyOf(session.localPlayerId()).hp;
        client.poll([&session](const network::MessageType type, const std::span<const std::byte> payload) {
            switch (type) {
                case network::MessageType::Welcome: {
                    const std::optional<network::WelcomeMessage> message =
                        network::PacketSerializer::deserialize<network::WelcomeMessage>(payload);
                    if (message.has_value()) {
                        if (const std::optional<domain::PlayerId> assigned =
                                playerIdFromByte(message->assignedPlayerId)) {
                            session.setLocalPlayerId(assigned.value());
                        }
                    }
                    break;
                }

                case network::MessageType::StateUpdate: {
                    const std::optional<network::StateUpdateMessage> message =
                        network::PacketSerializer::deserialize<network::StateUpdateMessage>(payload);
                    if (message.has_value()) {
                        applyStateUpdate(session, message.value());
                    }
                    break;
                }

                default:
                    break;
            }
        });

        if (wasConnected && !client.isConnected()) {
            session.world().setPhase(domain::MatchPhase::OpponentDisconnected);
        }

        if (input.shootPressed && session.world().phase() == domain::MatchPhase::Running) {
            app.triggerBulletTrail();
        }

        const int currentEnemyHp = session.world().enemyOf(session.localPlayerId()).hp;
        if (currentEnemyHp < previousEnemyHp) {
            app.triggerDamageNumber(previousEnemyHp - currentEnemyHp);
        }

        app.render(session.world(), session.localPlayerId());
    }

    return 0;
}

}  // namespace

int GameLoop::run(const std::filesystem::path& mapPath)
{
    presentation::SdlApp app;
    if (!app.initialize(DOOMLIKE_PROJECT_NAME, 1280, 720)) {
        return 1;
    }

    const std::optional<LaunchOptions> options = app.runStartupMenu();
    if (!options.has_value()) {
        return 0;
    }

    GameSession session;
    session.loadMap(mapPath);

    switch (options->mode) {
        case GameMode::Test:
            app.setHudModeLabel("test mode");
            session.setLocalPlayerId(domain::PlayerId::Host);
            session.startOfflineDemo();
            return runOfflineLoop(app, session);

        case GameMode::MultiplayerHost:
            app.setHudModeLabel("multiplayer host");
            return runHostLoop(app, session, options.value());

        case GameMode::MultiplayerClient:
            app.setHudModeLabel("multiplayer client");
            return runClientLoop(app, session, options.value());
    }

    return 0;
}

}  // namespace doomlike::application

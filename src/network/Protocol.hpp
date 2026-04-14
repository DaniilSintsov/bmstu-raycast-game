#pragma once

#include <cstdint>

namespace doomlike::network {

inline constexpr std::uint32_t kProtocolVersion = 1;
inline constexpr std::uint8_t kInvalidWinner = 255;

enum class MessageType : std::uint8_t {
    Connect = 1,
    Welcome = 2,
    InputCommand = 3,
    StateUpdate = 4,
    Shoot = 5,
    Hit = 6,
    Damage = 7,
    GameOver = 8,
    Restart = 9,
    Disconnect = 10,
};

enum class InputFlags : std::uint8_t {
    MoveForward = 1 << 0,
    MoveBackward = 1 << 1,
    StrafeLeft = 1 << 2,
    StrafeRight = 1 << 3,
    TurnLeft = 1 << 4,
    TurnRight = 1 << 5,
    Shoot = 1 << 6,
};

inline constexpr std::uint8_t toMask(const InputFlags value) noexcept
{
    return static_cast<std::uint8_t>(value);
}

#pragma pack(push, 1)

struct MessageHeader {
    MessageType type {};
    std::uint16_t size {0};
    std::uint32_t tick {0};
};

struct ConnectMessage {
    MessageHeader header {MessageType::Connect, 0, 0};
    std::uint32_t protocolVersion {kProtocolVersion};
};

struct WelcomeMessage {
    MessageHeader header {MessageType::Welcome, 0, 0};
    std::uint8_t assignedPlayerId {0};
    char mapName[32] {};
};

struct InputCommandMessage {
    MessageHeader header {MessageType::InputCommand, 0, 0};
    std::uint8_t playerId {0};
    std::uint8_t inputMask {0};
};

struct NetworkPlayerSnapshot {
    std::uint8_t playerId {0};
    float x {0.0F};
    float y {0.0F};
    float angleRadians {0.0F};
    std::int16_t hp {0};
    std::uint8_t alive {0};
};

struct StateUpdateMessage {
    MessageHeader header {MessageType::StateUpdate, 0, 0};
    NetworkPlayerSnapshot players[2] {};
    std::uint8_t phase {0};
    std::uint8_t winner {kInvalidWinner};
};

struct ShootMessage {
    MessageHeader header {MessageType::Shoot, 0, 0};
    std::uint8_t shooterId {0};
    float angleRadians {0.0F};
};

struct HitMessage {
    MessageHeader header {MessageType::Hit, 0, 0};
    std::uint8_t shooterId {0};
    std::uint8_t targetId {0};
};

struct DamageMessage {
    MessageHeader header {MessageType::Damage, 0, 0};
    std::uint8_t targetId {0};
    std::int16_t amount {0};
    std::int16_t newHp {0};
};

struct GameOverMessage {
    MessageHeader header {MessageType::GameOver, 0, 0};
    std::uint8_t winnerId {kInvalidWinner};
};

struct RestartMessage {
    MessageHeader header {MessageType::Restart, 0, 0};
};

struct DisconnectMessage {
    MessageHeader header {MessageType::Disconnect, 0, 0};
    std::uint8_t reasonCode {0};
};

#pragma pack(pop)

}  // namespace doomlike::network

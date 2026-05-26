#pragma once

#include <cstdint>
#include <string>

namespace doomlike::application {

inline constexpr std::uint16_t kDefaultNetworkPort = 27015;

enum class GameMode {
    Test,
    MultiplayerHost,
    MultiplayerClient,
};

struct LaunchOptions {
    GameMode mode {GameMode::Test};
    std::string hostIp {"127.0.0.1"};
    std::uint16_t port {kDefaultNetworkPort};
};

}  // namespace doomlike::application

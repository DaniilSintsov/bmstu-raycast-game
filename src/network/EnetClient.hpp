#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

#include <enet/enet.h>

#include "network/PacketSerializer.hpp"

namespace doomlike::network {

using PacketCallback = std::function<void(MessageType, std::span<const std::byte>)>;

class EnetClient {
public:
    EnetClient() = default;
    ~EnetClient();

    bool connect(std::string_view hostIp, std::uint16_t port, std::uint32_t timeoutMs = 3000);
    void disconnect();

    [[nodiscard]] bool isConnected() const noexcept;
    [[nodiscard]] bool sendBytes(std::span<const std::byte> bytes, bool reliable, std::uint8_t channel = 0);

    template <typename T>
    [[nodiscard]] bool send(const T& message, const bool reliable, const std::uint8_t channel = 0)
    {
        return sendBytes(PacketSerializer::serialize(message), reliable, channel);
    }

    void poll(const PacketCallback& callback, std::uint32_t timeoutMs = 0);

private:
    bool runtimeReady_ {false};
    ENetHost* host_ {nullptr};
    ENetPeer* peer_ {nullptr};
};

}  // namespace doomlike::network

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include <enet/enet.h>

#include "network/PacketCallback.hpp"
#include "network/PacketSerializer.hpp"

namespace doomlike::network {

class EnetHost {
public:
    EnetHost() = default;
    ~EnetHost();

    bool start(std::uint16_t port, std::size_t maxPeers = 1);
    void stop();

    [[nodiscard]] bool hasPeer() const noexcept;
    [[nodiscard]] bool sendToPeer(std::span<const std::byte> bytes, bool reliable, std::uint8_t channel = 0);
    [[nodiscard]] bool broadcastBytes(std::span<const std::byte> bytes, bool reliable, std::uint8_t channel = 0);

    template <typename T>
    [[nodiscard]] bool send(const T& message, const bool reliable, const std::uint8_t channel = 0)
    {
        return sendToPeer(PacketSerializer::serialize(message), reliable, channel);
    }

    void poll(const PacketCallback& callback, std::uint32_t timeoutMs = 0);

private:
    bool runtimeReady_ {false};
    ENetHost* host_ {nullptr};
    ENetPeer* peer_ {nullptr};
};

}  // namespace doomlike::network

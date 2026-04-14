#include "network/EnetHost.hpp"

#include "network/EnetRuntime.hpp"

namespace doomlike::network {

EnetHost::~EnetHost()
{
    stop();
}

bool EnetHost::start(const std::uint16_t port, const std::size_t maxPeers)
{
    stop();

    if (!runtimeReady_) {
        runtimeReady_ = EnetRuntime::acquire();
        if (!runtimeReady_) {
            return false;
        }
    }

    ENetAddress address {};
    address.host = ENET_HOST_ANY;
    address.port = port;

    host_ = enet_host_create(&address, maxPeers, 2, 0, 0);
    if (host_ == nullptr) {
        stop();
        return false;
    }

    return true;
}

void EnetHost::stop()
{
    if (host_ != nullptr) {
        enet_host_destroy(host_);
        host_ = nullptr;
        peer_ = nullptr;
    }

    if (runtimeReady_) {
        EnetRuntime::release();
        runtimeReady_ = false;
    }
}

bool EnetHost::hasPeer() const noexcept
{
    return peer_ != nullptr && peer_->state == ENET_PEER_STATE_CONNECTED;
}

bool EnetHost::sendToPeer(const std::span<const std::byte> bytes, const bool reliable, const std::uint8_t channel)
{
    if (!hasPeer() || bytes.empty()) {
        return false;
    }

    ENetPacket* packet =
        enet_packet_create(bytes.data(), bytes.size_bytes(), reliable ? ENET_PACKET_FLAG_RELIABLE : 0U);
    if (packet == nullptr) {
        return false;
    }

    if (enet_peer_send(peer_, channel, packet) != 0) {
        enet_packet_destroy(packet);
        return false;
    }

    return true;
}

bool EnetHost::broadcastBytes(const std::span<const std::byte> bytes, const bool reliable, const std::uint8_t channel)
{
    if (host_ == nullptr || bytes.empty()) {
        return false;
    }

    ENetPacket* packet =
        enet_packet_create(bytes.data(), bytes.size_bytes(), reliable ? ENET_PACKET_FLAG_RELIABLE : 0U);
    if (packet == nullptr) {
        return false;
    }

    enet_host_broadcast(host_, channel, packet);
    return true;
}

void EnetHost::poll(const PacketCallback& callback, std::uint32_t timeoutMs)
{
    if (host_ == nullptr) {
        return;
    }

    ENetEvent event {};
    while (enet_host_service(host_, &event, timeoutMs) > 0) {
        timeoutMs = 0;

        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                peer_ = event.peer;
                break;

            case ENET_EVENT_TYPE_RECEIVE: {
                const std::span<const std::byte> payload(
                    reinterpret_cast<const std::byte*>(event.packet->data),
                    event.packet->dataLength
                );

                if (PacketSerializer::validateSize(payload)) {
                    if (const auto type = PacketSerializer::peekType(payload)) {
                        callback(*type, payload);
                    }
                }

                enet_packet_destroy(event.packet);
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT:
                if (event.peer == peer_) {
                    peer_ = nullptr;
                }
                break;

            case ENET_EVENT_TYPE_NONE:
            default:
                break;
        }
    }
}

}  // namespace doomlike::network

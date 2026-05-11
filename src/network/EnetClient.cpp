#include "network/EnetClient.hpp"

#include <string>

#include "network/EnetRuntime.hpp"
#include "network/PacketSerializer.hpp"

namespace doomlike::network {

EnetClient::~EnetClient()
{
    disconnect();
}

bool EnetClient::connect(const std::string_view hostIp, const std::uint16_t port, const std::uint32_t timeoutMs)
{
    if (!runtimeReady_) {
        runtimeReady_ = EnetRuntime::acquire();
        if (!runtimeReady_) {
            return false;
        }
    }

    if (host_ == nullptr) {
        host_ = enet_host_create(nullptr, 1, 2, 0, 0);
        if (host_ == nullptr) {
            disconnect();
            return false;
        }
    }

    ENetAddress address {};
    address.port = port;
    const std::string ip(hostIp);
    if (enet_address_set_host(&address, ip.c_str()) != 0) {
        return false;
    }

    peer_ = enet_host_connect(host_, &address, 2, 0);
    if (peer_ == nullptr) {
        return false;
    }

    ENetEvent event {};
    const int serviced = enet_host_service(host_, &event, timeoutMs);
    if (serviced > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
        return true;
    }

    enet_peer_reset(peer_);
    peer_ = nullptr;
    return false;
}

void EnetClient::disconnect()
{
    if (peer_ != nullptr) {
        enet_peer_disconnect(peer_, 0);
        ENetEvent event {};
        while (enet_host_service(host_, &event, 100) > 0) {
            if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                enet_packet_destroy(event.packet);
            }
            if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
                break;
            }
        }
        peer_ = nullptr;
    }

    if (host_ != nullptr) {
        enet_host_destroy(host_);
        host_ = nullptr;
    }

    if (runtimeReady_) {
        EnetRuntime::release();
        runtimeReady_ = false;
    }
}

bool EnetClient::isConnected() const noexcept
{
    return peer_ != nullptr && peer_->state == ENET_PEER_STATE_CONNECTED;
}

bool EnetClient::sendBytes(const std::span<const std::byte> bytes, const bool reliable, const std::uint8_t channel)
{
    if (!isConnected() || bytes.empty()) {
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

    enet_host_flush(host_);
    return true;
}

void EnetClient::poll(const PacketCallback& callback, std::uint32_t timeoutMs)
{
    if (host_ == nullptr) {
        return;
    }

    ENetEvent event {};
    while (enet_host_service(host_, &event, timeoutMs) > 0) {
        timeoutMs = 0;

        if (event.type == ENET_EVENT_TYPE_RECEIVE) {
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
        } else if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
            peer_ = nullptr;
        }
    }
}

}  // namespace doomlike::network

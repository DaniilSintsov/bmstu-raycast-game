#include "network/PacketSerializer.hpp"

namespace doomlike::network {

std::optional<MessageType> PacketSerializer::peekType(const std::span<const std::byte> bytes)
{
    if (bytes.size_bytes() < sizeof(MessageHeader)) {
        return std::nullopt;
    }

    MessageHeader header {};
    std::memcpy(&header, bytes.data(), sizeof(MessageHeader));
    return header.type;
}

bool PacketSerializer::validateSize(const std::span<const std::byte> bytes)
{
    if (bytes.size_bytes() < sizeof(MessageHeader)) {
        return false;
    }

    MessageHeader header {};
    std::memcpy(&header, bytes.data(), sizeof(MessageHeader));
    return header.size == bytes.size_bytes();
}

}  // namespace doomlike::network

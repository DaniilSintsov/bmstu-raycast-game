#pragma once

#include <cstddef>
#include <cstring>
#include <optional>
#include <span>
#include <type_traits>
#include <vector>

#include "network/Protocol.hpp"

namespace doomlike::network {

class PacketSerializer {
public:
    template <typename T>
    [[nodiscard]] static std::vector<std::byte> serialize(T message)
    {
        static_assert(std::is_trivially_copyable_v<T>, "Network messages must be trivially copyable");
        message.header.size = static_cast<std::uint16_t>(sizeof(T));

        std::vector<std::byte> bytes(sizeof(T));
        std::memcpy(bytes.data(), &message, sizeof(T));
        return bytes;
    }

    template <typename T>
    [[nodiscard]] static std::optional<T> deserialize(const std::span<const std::byte> bytes)
    {
        static_assert(std::is_trivially_copyable_v<T>, "Network messages must be trivially copyable");

        if (bytes.size_bytes() != sizeof(T)) {
            return std::nullopt;
        }

        T message {};
        std::memcpy(&message, bytes.data(), sizeof(T));

        if (message.header.size != sizeof(T)) {
            return std::nullopt;
        }

        return message;
    }

    [[nodiscard]] static std::optional<MessageType> peekType(std::span<const std::byte> bytes);
    [[nodiscard]] static bool validateSize(std::span<const std::byte> bytes);
};

}  // namespace doomlike::network


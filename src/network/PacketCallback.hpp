#pragma once

#include <cstddef>
#include <functional>
#include <span>

#include "network/Protocol.hpp"

namespace doomlike::network {

using PacketCallback = std::function<void(MessageType, std::span<const std::byte>)>;

}  // namespace doomlike::network

#pragma once

#include "can_link.h"
#include <cstddef>
#include <cstdint>

using CanReceiveCallback = can_link_rx_handler_t;

inline int can_interface_init(
    CanReceiveCallback callback,
    void* user_data = nullptr)
{
    return can_link_init(callback, user_data);
}

// Returns 0 when the transfer starts, not when delivery completes.
// Returns -EBUSY if another transfer is already active.
inline int can_interface_send_to(
    std::uint8_t target_node,
    const std::uint8_t* payload,
    std::size_t length)
{
    return can_link_send_to(target_node, payload, length);
}

// The current SDSL-CAN implementation limits broadcasts to 7 bytes.
inline int can_interface_send_broadcast(
    const std::uint8_t* payload,
    std::size_t length)
{
    return can_link_send_broadcast(payload, length);
}
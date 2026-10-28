/** @file
 * @brief C++ adapter for the SDSL-CAN transport.
 */
#pragma once

#include "can_link.h"
#include <cstddef>
#include <cstdint>

/**
 * @brief Receive handler type supplied by SDSL-CAN.
 */
using CanReceiveCallback = can_link_rx_handler_t;

/**
 * @brief Register a receive callback and start SDSL-CAN.
 * @param callback Handler for received payloads.
 * @param user_data Context passed to the handler.
 * @return Result from can_link_init().
 */
inline int can_interface_init(
    CanReceiveCallback callback,
    void* user_data = nullptr)
{
    return can_link_init(callback, user_data);
}

/**
 * @brief Start a unicast transfer.
 * @param target_node Destination node address.
 * @param payload Bytes to send.
 * @param length Payload length in bytes.
 * @retval 0 Transfer started; delivery is not yet confirmed.
 * @retval -EBUSY Another transfer is active.
 * @return Other errors are passed through from can_link_send_to().
 */
inline int can_interface_send_to(
    std::uint8_t target_node,
    const std::uint8_t* payload,
    std::size_t length)
{
    return can_link_send_to(target_node, payload, length);
}

/**
 * @brief Send a broadcast of at most seven bytes.
 * @param payload Bytes to send.
 * @param length Payload length in bytes.
 * @return Result from can_link_send_broadcast().
 */
inline int can_interface_send_broadcast(
    const std::uint8_t* payload,
    std::size_t length)
{
    return can_link_send_broadcast(payload, length);
}

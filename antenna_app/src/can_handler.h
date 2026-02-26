#ifndef CAN_HANDLER_H
#define CAN_HANDLER_H

#include <zephyr/types.h>

#ifdef __cplusplus
/* Add this so main.cpp can see the semaphore */
extern struct k_sem blink_sem;
extern "C" {
#endif

/**
 * @brief Initializes the CAN controller and GPIO LED.
 * * Sets up the default RX filter to accept all standard data frames,
 * configures the onboard LED for activity toggling, and starts the 
 * CAN peripheral.
 *
 * @retval 0 if successful.
 * @retval -ENODEV if hardware devices are not ready.
 * @retval Negative errno code from zephyr CAN API on failure.
 */
int can_handler_init(void);

/**
 * @brief Transmits a classic CAN frame.
 *
 * @param id   The CAN ID (Standard).
 * @param data Pointer to the data buffer (max 8 bytes).
 * @param len  Length of data to send (clipped to 8).
 *
 * @retval 0 if successful.
 * @retval Negative errno code (e.g., -EAGAIN, -ENETDOWN) on failure.
 */
int can_handler_send(uint32_t id, uint8_t *data, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif /* CAN_HANDLER_H */
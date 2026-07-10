/**
 * @file can_handler.cpp
 * @brief CAN controller setup, receive callback, and transmit helper.
 *
 * CAN initialization and send failures are reported through the metrics fault
 * counter so system health telemetry can reflect bus setup/runtime problems.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>
#include "can_handler.h"
#include "metrics.h"

/* Use DT_CHOSEN for the CAN bus as it's more standard in Zephyr samples */
#define CAN_NODE DT_CHOSEN(zephyr_canbus)
#define LED_NODE DT_ALIAS(led0)

static const struct device *can_dev = DEVICE_DT_GET(CAN_NODE);
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

/* Add this near the top of can_handler.c */
K_SEM_DEFINE(blink_sem, 0, 1); 

/**
 * @brief Receive callback for accepted CAN frames.
 *
 * Current behavior treats CAN ID 0x123 as a simple blink command and releases
 * blink_sem for the main/application thread to consume.
 *
 * @param dev CAN device that received the frame.
 * @param frame Received CAN frame.
 * @param user_data Unused callback context.
 */
static void can_rx_callback(const struct device *dev, struct can_frame *frame, void *user_data)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(user_data);

    /* Check for a specific command ID, e.g., 0x123 */
    if (frame->id == 0x123) {
        k_sem_give(&blink_sem); // Signal the main thread to blink
    }
}

int can_handler_init(void)
{
    int ret;

    /* 1. Hardware Readiness Checks */
    if (!device_is_ready(can_dev)) {
        metrics_inc_fault();
        printk("CAN device not ready\n");
        return -ENODEV;
    }

    if (!gpio_is_ready_dt(&led)) {
        metrics_inc_fault();
        printk("LED GPIO device not ready\n");
        return -ENODEV;
    }

    /* 2. Configure LED */
    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        metrics_inc_fault();
        return ret;
    }

    /* 3. Setup RX Filter (Accept All Standard Data Frames) */
    const struct can_filter filter = {
        .id = 0,
        .mask = 0,
        .flags = 0, // In Zephyr 4.x, 0 handles standard data frames
    };

    /* can_add_rx_filter is the standard interface for receiving frames */
    ret = can_add_rx_filter(can_dev, can_rx_callback, NULL, &filter);
    if (ret < 0) {
        metrics_inc_fault();
        printk("Failed to add RX filter: %d\n", ret);
        return ret;
    }

    /* 4. Start the CAN controller */
    ret = can_start(can_dev);
    if (ret < 0) {
        metrics_inc_fault();
        printk("Failed to start CAN: %d\n", ret);
        return ret;
    }

    return 0;
}

int can_handler_send(uint32_t id, uint8_t *data, uint8_t len)
{
    struct can_frame frame;

    /* Initialize frame to zero to avoid garbage in flags/reserved bits */
    memset(&frame, 0, sizeof(struct can_frame));

    frame.id = id;
    frame.dlc = (len > 8) ? 8 : len; // Classic CAN max is 8
    memcpy(frame.data, data, frame.dlc);

    /* Send with a 100ms timeout for bus arbitration/ACK */
    int ret = can_send(can_dev, &frame, K_MSEC(100), NULL, NULL);

    if (ret < 0) {
        metrics_inc_fault();
    }

    return ret;
}

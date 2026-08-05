#pragma once

#include <stdint.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/devicetree.h>


typedef enum boot_check {
    spi_ready,
    gpio_ready,
    gpio_cs_ready,
    radio_init_ready,
    radio_thread_ready.
}; boot_check

void check_spi();
/**
     * @brief Check if spi is ready
     * return false if spi not ready
     */

bool check_gpio();
/**
     * @brief check if gpio is ready
     * return false gpio if not ready
     */

bool check_gpio_cs();
/**
     * @brief Check if gpio_cs is ready
     * return false if gpio_cs is not ready
     */

bool check_CAN();
/**
     * @brief check if CAN is ready
     * return false if CAN is not ready
     */

bool check_radio_init();
/**
     * @brief Check if radio_init is ready
     * return false if radio_inti is not ready
     */

void check_radio_thread();
/**
     * @brief Check if radio_thread is ready
     * return false if radio_thread is not ready
     */

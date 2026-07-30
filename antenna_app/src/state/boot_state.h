#pragma once
#include <stdint.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/device.h>

typedef struct board_init {
    uint8_t board_id;
    struct i2c_dt_spec i2c;
    struct gpio_dt_spec gpio_pin;
    const struct device *can_bus;
} board_init_t;
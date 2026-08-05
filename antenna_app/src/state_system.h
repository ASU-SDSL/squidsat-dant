#include <stdint.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/devicetree.h>


struct device_configuration{
    const device* i2c_port;
    const device* spi_port;
    const device* gpio_port;
    const device* gpio_cs_port;
    const device* can_bus;
};

extern const struct device_configuration device_config; 
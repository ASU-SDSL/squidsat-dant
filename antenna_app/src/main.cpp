#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include "can_handler.h"

LOG_MODULE_REGISTER(main_app, LOG_LEVEL_INF);

/* Devicetree Hardware Pins */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

int main() {
    int ret;
    uint8_t cmd_data[] = {0x01}; // Simple "Blink" command

    /* Initialize Hardware */
    can_handler_init();
    
    if (!gpio_is_ready_dt(&led) || !gpio_is_ready_dt(&btn)) {
        LOG_ERR("GPIO Hardware not ready");
        return 0;
    }

    gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&btn, GPIO_INPUT);

    LOG_INF("Press B1 (Blue Button) to send CAN message!");

    while (true) {
        /* --- TRANSMITTER LOGIC --- */
        /* If button is pressed (B1 is active-low on Nucleo, 
           but gpio_pin_get_dt handles the logic polarity for you) */
        if (gpio_pin_get_dt(&btn) > 0) {
            gpio_pin_set_dt(&led, 1); // Light up LED locally
            
            ret = can_handler_send(0x123, cmd_data, sizeof(cmd_data));
            if (ret == 0) {
                LOG_INF("Button Pressed: CAN message sent!");
            }
            
            /* Debounce delay so we don't spam the bus */
            k_msleep(200); 
            gpio_pin_set_dt(&led, 0);
        }

        /* --- RECEIVER LOGIC --- */
        /* Check if the CAN callback signaled us to blink (1ms timeout) */
        if (k_sem_take(&blink_sem, K_MSEC(1)) == 0) {
            LOG_INF("Received CAN command! Blinking 10 times...");
            for (int i = 0; i < 20; i++) { // 20 toggles = 10 blinks
                gpio_pin_toggle_dt(&led);
                k_msleep(100);
            }
        }

        k_msleep(10); // Small yield to prevent CPU hogging
    }
    return 0;
}
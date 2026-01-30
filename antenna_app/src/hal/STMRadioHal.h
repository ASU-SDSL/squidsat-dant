
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <stdio.h>
#include <stm32f1xx_hal_gpio.h>

#include <stm32f101x6.h>
#include "RadioLib.h"

class STMRadioHal : public RadioLibHal {
public:
    STMRadioHal(const struct device* can_dev, const struct device* gpio_dev, uint32_t input,
           uint32_t output,
           uint32_t low, uint32_t high,
           uint32_t rising, uint32_t falling)
    : RadioLibHal(input, output, low, high, rising, falling), 
    can_dev(can_dev),
    gpio_dev(gpio_dev)
    {
        printf("Radio HAL initialized\n");
    }

    void init() override {
        // Zephyr CAN configuration is typically done via device tree, not struct
        // These functions work with the device from device tree
        can_set_mode(can_dev, CAN_MODE_NORMAL);
        can_start(can_dev);
    }

    void term() override {
        can_stop(can_dev);
    }

    void pinMode(uint32_t pin, uint32_t value) override {
        if(pin == RADIOLIB_NC){return;}
        // how do i initialize the pins for can?
    }

    void digitalWrite(uint32_t pin, uint32_t value) override {
        if (pin == RADIOLIB_NC) {return;}
        if (pin > 8){return;} 
        
        // Use Zephyr GPIO API instead of STM32 HAL
        gpio_pin_set(gpio_dev, pin, value ? 1 : 0);
    }

    private:
        const struct device* can_dev;  
        const struct device* gpio_dev;
};
;

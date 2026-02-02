
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <stdio.h>
#include <stm32f1xx_hal_gpio.h>

#include <stm32f101x6.h>
#include "RadioLib.h"

class STMRadioHal : public RadioLibHal {

public:
    STMRadioHal(const struct device* spi, uint32_t spi_speed = 2000000)
    : RadioLibHal(0, 1, 0, 1, 0, 1), 
    _spi(spi), _spi_speed(spi_speed),
    gpio_dev(gpio_dev)
    {
        printf("Radio HAL initialized\n");
    }

    void init() override {
        // Zephyr CAN configuration is typically done via device tree, not struct
        // These functions work with the device from device tree
        spiBegin();
    }

    void term() override {
        spiEnd();
    }

    void pinMode(uint32_t pin, uint32_t value) override {
        if(pin == RADIOLIB_NC){return;}
        // how do i initialize the pins for can?
    }

    void digitalWrite(uint32_t pin, uint32_t value) override {
        if (pin == RADIOLIB_NC) {return;}
        
        // Use Zephyr GPIO API instead of STM32 HAL
        gpio_pin_set(gpio_dev, pin, value);
    }

    uint32_t digitalRead(uint32_t pin) override {
        if (pin == RADIOLIB_NC) {
            return 0;
        }

        return gpio_pin_get(gpio_dev, pin);
    } 

    typedef void(* 	gpio_callback_handler_t) (const struct device *port, struct gpio_callback *cb, gpio_port_pins_t pins);
    void attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void), uint32_t mode) override {
        // Zephyr GPIO interrupt setup
        // This is a simplified example; actual implementation may vary
        if(interruptNum == RADIOLIB_NC){return;}

        gpio_init_callback(&gpio_cb_data, reinterpret_cast<gpio_callback_handler_t>(interruptCb), BIT(interruptNum));
        gpio_add_callback(gpio_dev, &gpio_cb_data);
        gpio_pin_interrupt_configure(gpio_dev, interruptNum, mode);
    }

    void detachInterrupt(uint32_t interruptNum) override {
        if (interruptNum == RADIOLIB_NC) {
            return;
        }
        // Disabling the interrupt with the Zephyr GPIO API
        gpio_pin_interrupt_configure(gpio_dev, interruptNum, GPIO_INT_DISABLE);
        gpio_remove_callback(gpio_dev, &gpio_cb_data);
    }

    void delay(unsigned long ms) override {
        k_msleep(ms);
    }

    void delayMicroseconds(unsigned long us) override {
        k_msleep(us / 1000); // pretty sure this is meant to specify sleep in microseconds
    }

    unsigned long millis() override {
        return k_uptime_get();
    }

    unsigned long micros() override {
        return k_uptime_get() * 1000;
    }
    
    long pulseIn(uint32_t pin, uint32_t state, unsigned long timeout) override {
        printf("pulseIn\n");
        if (pin == RADIOLIB_NC) {
            return 0;
        }

        gpio_pin_configure(gpio_dev, pin, GPIO_INPUT);
        uint32_t start = this->micros();
        uint32_t curtick = this->micros();
        uint32_t timeoutMicros = timeout; // * 1000; 

        while (gpio_pin_get(gpio_dev, pin) == state) {
            if ((this->micros() - curtick) > timeoutMicros) {
                return 0;
            }
        }

        return (this->micros() - start);
    }

    void spiBegin() override {
        printf("spiBegin\n");
        _spi_cfg.frequency = _spi_speed;
        _spi_cfg.operation =
            SPI_WORD_SET(8) |
            SPI_TRANSFER_MSB |
            SPI_MODE_CPOL |
            SPI_MODE_CPHA;
        _spi_cfg.slave = 0;
        _spi_cfg.cs.gpio.port = nullptr;
    }

    private:
        const uint32_t _spi_speed;
        const struct device *_spi;
        struct spi_config _spi_cfg;
        int _spiHandle = -1;
        struct gpio_callback gpio_cb_data;
        const struct device* gpio_dev;
    };
;

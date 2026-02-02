
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <stdio.h>
#include <stm32f1xx_hal_gpio.h>

#include <stm32f101x6.h>
#include "RadioLib.h"

#include <map>

static std::map<uint32_t, void (*)(void)> isrs;
static void zephyrGeneralISR(const struct device *dev, struct gpio_callback *cb, uint32_t pins){
    for (auto &entry : isrs) {
        uint32_t pin = entry.first;
        if (pins & BIT(pin)) {
            entry.second();  
        }
    }
}

class STMRadioHal : public RadioLibHal {
public:
    STMRadioHal(const struct device* spi, const struct device* gpio, uint32_t spi_speed = 2000000)
    : RadioLibHal(GPIO_INPUT, GPIO_OUTPUT, 0, 1, GPIO_INT_EDGE_RISING, GPIO_INT_EDGE_FALLING), 
    _spi(spi), _spi_speed(spi_speed),
    gpio_dev(gpio)
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
        gpio_pin_configure(gpio_dev, pin, value);
    }

    void digitalWrite(uint32_t pin, uint32_t value) override {
        if (pin == RADIOLIB_NC) {return;}
        
        // may not work
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
        if (interruptNum == RADIOLIB_NC) return;

        isrs[interruptNum] = interruptCb;

        gpio_pin_configure(gpio_dev, interruptNum, GPIO_INPUT);
        gpio_pin_interrupt_configure(gpio_dev, interruptNum, mode);

        uint32_t mask = 0;
        for (auto &e : isrs) {
            mask |= BIT(e.first);
        }

        gpio_remove_callback(gpio_dev, &gpio_cb_data);
        gpio_init_callback(&gpio_cb_data, zephyrGeneralISR, mask);
        gpio_add_callback(gpio_dev, &gpio_cb_data);
    }

    void detachInterrupt(uint32_t interruptNum) override {
        if (interruptNum == RADIOLIB_NC) {
            return;
        }
        // Disabling the interrupt with the Zephyr GPIO API
        gpio_pin_interrupt_configure(gpio_dev, interruptNum, GPIO_INT_DISABLE);
        gpio_remove_callback(gpio_dev, &gpio_cb_data);
        isrs.erase(interruptNum);
    }

    void delay(unsigned long ms) override {
        k_msleep(ms);
    }

    void delayMicroseconds(unsigned long us) override {
        k_busy_wait(us); // pretty sure this is meant to specify sleep in microseconds
    }

    unsigned long millis() override {
        return k_uptime_get();
    }

    unsigned long micros() override {
        return k_uptime_get() * 1000;
    }
    
    long pulseIn(uint32_t pin, uint32_t state, unsigned long timeout) override {
        if (pin == RADIOLIB_NC) {
            return 0;
        }

        gpio_pin_configure(gpio_dev, pin, GPIO_INPUT);
        uint32_t start = micros();

        while (gpio_pin_get(gpio_dev, pin) == (int)state) {
            if ((micros() - start) > timeout) {
                return 0;
            }
        }
    
        return micros() - start;
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
        struct gpio_callback gpio_cb_data;
        const struct device* gpio_dev;
    };
;

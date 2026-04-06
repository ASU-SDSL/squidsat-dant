
#pragma once

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <stdio.h>
#include <stm32f1xx_hal_gpio.h>

#include "RadioLib.h"

struct isr_entry_t {
    uint32_t pin;
    void (*handler)(void);
};

static isr_entry_t isrs[8];
static uint8_t isr_count = 0;

static void zephyrGeneralISR(const struct device *dev, struct gpio_callback *cb, uint32_t pins){
    ARG_UNUSED(dev);
    ARG_UNUSED(cb);
    for (uint8_t i = 0; i < isr_count; i++) {
        if ((pins & BIT(isrs[i].pin)) && (isrs[i].handler != nullptr)) {
            isrs[i].handler();
        }
    }
}

class STMRadioHal : public RadioLibHal {
public:
    STMRadioHal(const struct device* spi, const struct device* gpio, uint32_t spi_speed = 2000000)
    : RadioLibHal(GPIO_INPUT, GPIO_OUTPUT, 0, 1, GPIO_INT_EDGE_RISING, GPIO_INT_EDGE_FALLING), 
    _spi_speed(spi_speed), _spi(spi), _spi_cfg{},
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
        
        gpio_pin_set(gpio_dev, pin, value);
    }

    uint32_t digitalRead(uint32_t pin) override {
        if (pin == RADIOLIB_NC) {
            return 0;
        }

        return gpio_pin_get(gpio_dev, pin);
    } 

    void attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void), uint32_t mode) override {
        if (interruptNum == RADIOLIB_NC) return;

        bool updated = false;
        for (uint8_t i = 0; i < isr_count; i++) {
            if (isrs[i].pin == interruptNum) {
                isrs[i].handler = interruptCb;
                updated = true;
                break;
            }
        }
        if (!updated && isr_count < ARRAY_SIZE(isrs)) {
            isrs[isr_count].pin = interruptNum;
            isrs[isr_count].handler = interruptCb;
            isr_count++;
        }

        gpio_pin_configure(gpio_dev, interruptNum, GPIO_INPUT);
        gpio_pin_interrupt_configure(gpio_dev, interruptNum, mode);

        uint32_t mask = 0;
        for (uint8_t i = 0; i < isr_count; i++) {
            mask |= BIT(isrs[i].pin);
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
        for (uint8_t i = 0; i < isr_count; i++) {
            if (isrs[i].pin == interruptNum) {
                for (uint8_t j = i; j + 1 < isr_count; j++) {
                    isrs[j] = isrs[j + 1];
                }
                isr_count--;
                break;
            }
        }
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
        _spi_cfg = {};
        _spi_cfg.frequency = _spi_speed;
        _spi_cfg.operation =
            SPI_WORD_SET(8) |
            SPI_TRANSFER_MSB;
        _spi_cfg.slave = 0;
        _spi_cfg.cs.cs_is_gpio = false; // leave as false, handled by digitalWrite() and other methods 
        _spi_cfg.cs.gpio.port = nullptr;
    }

    void spiBeginTransaction() override {
        // Chip select is driven by RadioLib using digitalWrite().
    }

    void spiTransfer(uint8_t* out, size_t len, uint8_t* in) override {
        if ((out == nullptr) || (len == 0)) {
            return;
        }

        struct spi_buf tx_buf = {
            .buf = out,
            .len = len,
        };
        struct spi_buf_set tx_set = {
            .buffers = &tx_buf,
            .count = 1,
        };

        if (in != nullptr) {
            struct spi_buf rx_buf = {
                .buf = in,
                .len = len,
            };
            struct spi_buf_set rx_set = {
                .buffers = &rx_buf,
                .count = 1,
            };
            (void)spi_transceive(_spi, &_spi_cfg, &tx_set, &rx_set);
        } else {
            (void)spi_write(_spi, &_spi_cfg, &tx_set);
        }
    }

    void spiEndTransaction() override {
        // No transaction teardown required for Zephyr SPI API.
    }

    void spiEnd() override {
        // Nothing to deinitialize for Zephyr SPI device handle.
    }

    private:
        const uint32_t _spi_speed;
        const struct device *_spi;
        struct spi_config _spi_cfg;
        struct gpio_callback gpio_cb_data;
        const struct device* gpio_dev;
    };
;

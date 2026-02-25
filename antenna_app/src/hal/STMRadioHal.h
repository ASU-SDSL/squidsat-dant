
#pragma once

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <stdio.h>
#include <string.h>
// #include <stm32f1xx_hal_gpio.h>

// #include <stm32f101x6.h>
#include <RadioLib.h>

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
        // Initialize SPI config with safe defaults
        memset(&_spi_cfg, 0, sizeof(_spi_cfg));
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

    void pinMode(uint32_t pin, uint32_t mode) override {
        if(pin == RADIOLIB_NC){return;}
        
        gpio_flags_t flags = 0;
        if (mode == GPIO_OUTPUT) {
            flags = GPIO_OUTPUT;
        } else if (mode == GPIO_INPUT) {
            flags = GPIO_INPUT;
        }
        gpio_pin_configure(gpio_dev, pin, flags);
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
        //return (unsigned long)k_ticks_to_us_near64(k_uptime_ticks());
        return (unsigned long)(k_uptime_get() * 1000ULL);
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

        uint32_t pulse_start = micros();
        while (gpio_pin_get(gpio_dev, pin) == (int)state) {
            if ((micros() - pulse_start) > timeout) return 0;
        }
    
        return (long)(micros() - pulse_start);
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
        // Disable automatic GPIO CS control - RadioLib handles CS manually
        _spi_cfg.cs.cs_is_gpio = false;
    }

    
    void spiBeginTransaction() override {
    }
    void spiEndTransaction() override {
    }

    void spiTransfer(uint8_t *out, size_t len, uint8_t *in) override {
        struct spi_buf tx_buf = {
            .buf = out,
            .len = len,
        };

    /* Describe RX memory */
        struct spi_buf rx_buf = {
            .buf = in,
            .len = len,
        };

    /* Wrap buffers into sets */
        struct spi_buf_set tx_set = {
            .buffers = &tx_buf,
            .count = 1,
        };

        struct spi_buf_set rx_set = {
            .buffers = &rx_buf,
            .count = 1,
        };

        spi_transceive(_spi, &_spi_cfg, &tx_set, &rx_set);
    }

    void spiEnd() override {
        // I read that the spi is managed by the kernel and doesn't need to be ended.
    }

    private:
        const uint32_t _spi_speed;
        const struct device *_spi;
        struct spi_config _spi_cfg;
        struct gpio_callback gpio_cb_data;
        const struct device* gpio_dev;
    };
;


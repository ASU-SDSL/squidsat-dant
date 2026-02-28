#pragma once

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <stdio.h>
#include <string.h>
#include <RadioLib.h>

#define MAX_ISRS 8

struct ISREntry {
    uint32_t pin;
    void (*cb)(void);
    bool used;
};

static ISREntry isrs[MAX_ISRS];

static void zephyrGeneralISR(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    for (auto &e : isrs) {
        if (e.used && (pins & BIT(e.pin))) {
            e.cb();
        }
    }
}

class STMRadioHal : public RadioLibHal {
public:
    STMRadioHal(const struct device* spi,
                const struct device* gpio,
                const struct device* cs_gpio,
                uint32_t cs_pin,
                uint32_t spi_speed = 2000000)
    : RadioLibHal(GPIO_INPUT, GPIO_OUTPUT, 0, 1, GPIO_INT_EDGE_RISING, GPIO_INT_EDGE_FALLING),
    _spi_speed(spi_speed), _spi(spi),
    _cs_pin(cs_pin), gpio_dev(gpio), cs_gpio_dev(cs_gpio)
    {
        memset(&_spi_cfg, 0, sizeof(_spi_cfg));
        memset(isrs, 0, sizeof(isrs));
        printf("Radio HAL initialized\n");
    }

    void init() override {
        spiBegin();
    }

    void term() override {
        spiEnd();
    }

    void pinMode(uint32_t pin, uint32_t mode) override {
        if (pin == RADIOLIB_NC) { return; }

        gpio_flags_t flags = 0;
        if (mode == GPIO_OUTPUT) {
            flags = GPIO_OUTPUT;
        } else if (mode == GPIO_INPUT) {
            flags = GPIO_INPUT;
        }

        const struct device* dev = (pin == _cs_pin) ? cs_gpio_dev : gpio_dev;
        gpio_pin_configure(dev, pin, flags);
    }

    void digitalWrite(uint32_t pin, uint32_t value) override {
        if (pin == RADIOLIB_NC) { return; }

        const struct device* dev = (pin == _cs_pin) ? cs_gpio_dev : gpio_dev;
        gpio_pin_set(dev, pin, value);
    }

    uint32_t digitalRead(uint32_t pin) override {
        if (pin == RADIOLIB_NC) { return 0; }

        const struct device* dev = (pin == _cs_pin) ? cs_gpio_dev : gpio_dev;
        return gpio_pin_get(dev, pin);
    }

    void attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void), uint32_t mode) override {
        if (interruptNum == RADIOLIB_NC) return;

        // Insert or update entry
        for (auto &e : isrs) {
            if (!e.used || e.pin == interruptNum) {
                e.pin = interruptNum;
                e.cb = interruptCb;
                e.used = true;
                break;
            }
        }

        gpio_pin_configure(gpio_dev, interruptNum, GPIO_INPUT);
        gpio_pin_interrupt_configure(gpio_dev, interruptNum, mode);

        // Rebuild pin mask
        uint32_t mask = 0;
        for (auto &e : isrs) {
            if (e.used) mask |= BIT(e.pin);
        }

        gpio_remove_callback(gpio_dev, &gpio_cb_data);
        gpio_init_callback(&gpio_cb_data, zephyrGeneralISR, mask);
        gpio_add_callback(gpio_dev, &gpio_cb_data);
    }

    void detachInterrupt(uint32_t interruptNum) override {
        if (interruptNum == RADIOLIB_NC) { return; }

        gpio_pin_interrupt_configure(gpio_dev, interruptNum, GPIO_INT_DISABLE);
        gpio_remove_callback(gpio_dev, &gpio_cb_data);

        // Remove entry
        for (auto &e : isrs) {
            if (e.used && e.pin == interruptNum) {
                e.used = false;
                break;
            }
        }
    }

    void delay(unsigned long ms) override {
        k_msleep(ms);
    }

    void delayMicroseconds(unsigned long us) override {
        k_busy_wait(us);
    }

    unsigned long millis() override {
        return k_uptime_get();
    }

    unsigned long micros() override {
        return (unsigned long)(k_uptime_get() * 1000ULL);
    }

    long pulseIn(uint32_t pin, uint32_t state, unsigned long timeout) override {
        if (pin == RADIOLIB_NC) { return 0; }

        gpio_pin_configure(gpio_dev, pin, GPIO_INPUT);
        uint32_t start = micros();

        while (gpio_pin_get(gpio_dev, pin) == (int)state) {
            if ((micros() - start) > timeout) { return 0; }
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
            SPI_TRANSFER_MSB;
        _spi_cfg.slave = 0;
    }

    void spiBeginTransaction() override {}
    void spiEndTransaction() override {}

    void spiTransfer(uint8_t *out, size_t len, uint8_t *in) override {
        if (len == 0U) {
            return;
        }

        if ((out != nullptr) && (in != nullptr)) {
            struct spi_buf tx_buf = { .buf = out, .len = len };
            struct spi_buf rx_buf = { .buf = in,  .len = len };
            struct spi_buf_set tx_set = { .buffers = &tx_buf, .count = 1 };
            struct spi_buf_set rx_set = { .buffers = &rx_buf, .count = 1 };

            int err = spi_transceive(_spi, &_spi_cfg, &tx_set, &rx_set);
            if (err != 0) {
                printf("spi_transceive failed: %d\n", err);
            }
            return;
        }

        // Handle half-duplex-like use safely when RadioLib passes null out/in pointers.
        for (size_t i = 0; i < len; i++) {
            uint8_t tx = (out != nullptr) ? out[i] : 0xFF;
            uint8_t rx = 0x00;
            struct spi_buf tx_buf = { .buf = &tx, .len = 1 };
            struct spi_buf rx_buf = { .buf = &rx, .len = 1 };
            struct spi_buf_set tx_set = { .buffers = &tx_buf, .count = 1 };
            struct spi_buf_set rx_set = { .buffers = &rx_buf, .count = 1 };

            int err = spi_transceive(_spi, &_spi_cfg, &tx_set, &rx_set);
            if (err != 0) {
                printf("spi_transceive failed: %d at byte %u\n", err, (unsigned)i);
                return;
            }
            if (in != nullptr) {
                in[i] = rx;
            }
        }
    }

    void spiEnd() override {}

private:
    const uint32_t _spi_speed;
    const struct device *_spi;
    const uint32_t _cs_pin;
    struct spi_config _spi_cfg;
    struct gpio_callback gpio_cb_data;
    const struct device *gpio_dev;
    const struct device *cs_gpio_dev;
};

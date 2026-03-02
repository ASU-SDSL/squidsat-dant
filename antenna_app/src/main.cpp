#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>

#include <RadioLib.h>
#include <STMRadioHal.h>
#include <zephyr/drivers/spi.h>
#include <string.h>

#define SPI_DEV    DT_NODELABEL(spi1)
#define GPIO_NODE  DT_NODELABEL(gpioa)
#define GPIO_CS_NODE DT_NODELABEL(gpioa)
#define CS_PIN     4
#define IRQ_PIN    8
#define RST_PIN    9
#define BUSY_PIN   10

const struct device *const spi_bus   = DEVICE_DT_GET(SPI_DEV);
const struct device *const gpio_port = DEVICE_DT_GET(GPIO_NODE);
const struct device *const gpio_cs_port = DEVICE_DT_GET(GPIO_CS_NODE);

int count = 0;
volatile bool tx_done = false;

void onTxDone(void)
{
    tx_done = true;
}

int main(void)
{
    if (!device_is_ready(spi_bus)) {
        printk("SPI bus not ready\n");
        return 0;
    }

    if (!device_is_ready(gpio_port)) {
        printk("GPIO port not ready\n");
        return 0;
    }

    if (!device_is_ready(gpio_cs_port)) {
        printk("GPIO CS port not ready\n");
        return 0;
    }

    // Pre-init diagnostics for SX1268 control lines.
    gpio_pin_configure(gpio_cs_port, CS_PIN, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure(gpio_port, IRQ_PIN, GPIO_INPUT);
    gpio_pin_configure(gpio_port, BUSY_PIN, GPIO_INPUT);
    gpio_pin_configure(gpio_port, RST_PIN, GPIO_OUTPUT_ACTIVE);
    gpio_pin_set(gpio_port, RST_PIN, 1);
    k_msleep(5);
    printk("[SX1268] pins before reset: DIO1=%d BUSY=%d NRST=%d\n",
           gpio_pin_get(gpio_port, IRQ_PIN),
           gpio_pin_get(gpio_port, BUSY_PIN),
           gpio_pin_get(gpio_port, RST_PIN));

    // Hardware reset pulse.
    gpio_pin_set(gpio_port, RST_PIN, 0);
    k_msleep(2);
    gpio_pin_set(gpio_port, RST_PIN, 1);
    k_msleep(10);
    printk("[SX1268] pins after reset: DIO1=%d BUSY=%d NRST=%d\n",
           gpio_pin_get(gpio_port, IRQ_PIN),
           gpio_pin_get(gpio_port, BUSY_PIN),
           gpio_pin_get(gpio_port, RST_PIN));

    // Low-level probe: SX126x GET_STATUS command (0xC0), sweep SPI modes.
    static const uint16_t mode_bits[4] = {
        0,
        SPI_MODE_CPHA,
        SPI_MODE_CPOL,
        SPI_MODE_CPOL | SPI_MODE_CPHA
    };
    for (int mode = 0; mode < 4; mode++) {
        struct spi_config probe_cfg = {};
        probe_cfg.frequency = 1000000;  // STM32F1 SPI1 min is ~281 kHz
        probe_cfg.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | mode_bits[mode];
        probe_cfg.slave = 0;

        uint8_t tx_probe[3] = {0xC0, 0x00, 0x00};
        uint8_t rx_probe[3] = {0x00, 0x00, 0x00};
        struct spi_buf tx_buf = {.buf = tx_probe, .len = sizeof(tx_probe)};
        struct spi_buf rx_buf = {.buf = rx_probe, .len = sizeof(rx_probe)};
        struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};
        struct spi_buf_set rx_set = {.buffers = &rx_buf, .count = 1};

        gpio_pin_set(gpio_cs_port, CS_PIN, 0);
        k_busy_wait(5);
        int probe_err = spi_transceive(spi_bus, &probe_cfg, &tx_set, &rx_set);
        k_busy_wait(5);
        gpio_pin_set(gpio_cs_port, CS_PIN, 1);

        printk("[SX1268] probe mode=%d err=%d rx=[0x%02x 0x%02x 0x%02x] busy=%d\n",
               mode, probe_err, rx_probe[0], rx_probe[1], rx_probe[2],
               gpio_pin_get(gpio_port, BUSY_PIN));
    }

    STMRadioHal stm_hal(spi_bus, gpio_port, gpio_cs_port, CS_PIN, 2000000);
    Module module(&stm_hal, CS_PIN, IRQ_PIN, RST_PIN, BUSY_PIN);
    SX1268 radio(&module);

    printk("[SX1268] Initializing...\n");
    int state = radio.begin(
        434.0,  // frequency MHz
        125.0,  // bandwidth kHz
        9,      // spreading factor
        7,      // coding rate
        0x12,   // sync word
        10,     // output power dBm
        8,      // preamble length
        0       // TCXO voltage (0 = disabled)
    );
    if (state != RADIOLIB_ERR_NONE) {
        printk("[SX1268] Init failed, code %d\n", state);
        return 0;
    }
    printk("[SX1268] Init success!\n");

    radio.setPacketSentAction(onTxDone);

    char msg[64];
    snprintf(msg, sizeof(msg), "Hello World! #%d", count++);
    state = radio.startTransmit(msg);
    if (state != RADIOLIB_ERR_NONE) {
        printk("[SX1268] startTransmit failed, code %d\n", state);
        return 0;
    }
    printk("[SX1268] First packet started\n");

    while (1) {
        if (!tx_done) {
            k_msleep(10);
            continue;
        }

        tx_done = false;
        state = radio.finishTransmit();
        if (state == RADIOLIB_ERR_NONE) {
            printk("[SX1268] TX done\n");
        } else {
            printk("[SX1268] finishTransmit failed, code %d\n", state);
        }

        k_msleep(1000);

        snprintf(msg, sizeof(msg), "Hello World! #%d", count++);
        state = radio.startTransmit(msg);
        if (state == RADIOLIB_ERR_NONE) {
            printk("[SX1268] TX started\n");
        } else if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
            printk("[SX1268] TX start failed: too long\n");
        } else if (state == RADIOLIB_ERR_TX_TIMEOUT) {
            printk("[SX1268] TX start failed: timeout\n");
        } else {
            printk("[SX1268] TX start failed, code %d\n", state);
        }
    }
}

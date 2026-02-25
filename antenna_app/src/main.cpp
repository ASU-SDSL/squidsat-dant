#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <RadioLib.h>
#include <STMRadioHal.h>   // your RadioLibHal implementation
#include <zephyr/drivers/spi.h>

#define SPI_DEV DT_NODELABEL(spi1)
#define GPIO_NODE DT_NODELABEL(gpioa)
#define CS_PIN    10  // SPI chip select for SX1262
#define IRQ_PIN    2  // DIO1 / IRQ
#define RST_PIN    3  // Reset
#define BUSY_PIN   4  // Busy

const struct device *const spi_bus = DEVICE_DT_GET(SPI_DEV);
const struct device *const gpio_port = DEVICE_DT_GET(GPIO_NODE);

int count = 0;

int main(void)
{
    STMRadioHal stm_hal(spi_bus, gpio_port, 2000000);

    Module module(&stm_hal, CS_PIN, IRQ_PIN, RST_PIN, BUSY_PIN);
    SX1262 radio(&module);
    while (1) {

        printk("[SX1262] Transmitting packet...\n");

        char msg[64];
        snprintf(msg, sizeof(msg),
                 "Hello World! #%d", count++);

        int state = radio.transmit(msg);

        if (state == RADIOLIB_ERR_NONE) {
            printk("success!\n");
        } else if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
            printk("too long!\n");
        } else if (state == RADIOLIB_ERR_TX_TIMEOUT) {
            printk("timeout!\n");
        } else {
            printk("failed, code %d\n", state);
        }

        k_msleep(1000);
    }
}
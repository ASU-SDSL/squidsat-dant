/*
 * DANT REV1 burn wire test.
 *
 * All deployment gates are driven low at boot. A gate is energized only when
 * commanded over USART1 (115200 8N1), and only one gate at a time, for a
 * bounded duration. Any received byte while a gate is on aborts the burn.
 *
 * Commands (end with Enter):
 *   l            list gates
 *   f <n> [ms]   fire gate n (0-15) for ms (default 2000, max 10000)
 *   x            force all gates low
 */
#include <stdlib.h>
#include <string.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/ring_buffer.h>

#define BURN_DEFAULT_MS 2000
#define BURN_MAX_MS     10000

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct device *const uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

/*
 * USART1 holds a single received byte, so polling it from a sleeping loop
 * drops pasted or scripted input. The RX interrupt moves every byte into this
 * ring buffer; the command loop and the burn abort check read from it.
 */
RING_BUF_DECLARE(rx_ring, 128);

static void uart_rx_isr(const struct device *dev, void *user_data)
{
    ARG_UNUSED(user_data);

    while (uart_irq_update(dev) && uart_irq_rx_ready(dev)) {
        uint8_t buf[16];
        int n = uart_fifo_read(dev, buf, sizeof(buf));

        if (n <= 0) {
            break;
        }
        ring_buf_put(&rx_ring, buf, n); /* drops bytes only if 128 are pending */
    }
}

/* Returns 0 and stores a byte in *c if one is pending, -1 otherwise. */
static int rx_get(unsigned char *c)
{
    return ring_buf_get(&rx_ring, c, 1) == 1 ? 0 : -1;
}

/*
 * Deployment gates, from the REV1 STM32 and Burn Resistors schematic sheets.
 * Each gate drives an SSF2318E low-side MOSFET. All share one LT3083 burn
 * supply ("Regulator Output"), so only one gate may be on at a time.
 *   Antenna:     BURN1-8 switch 2 ohm burn resistors R14A-R17B on the board.
 *   Solar panel: SP_BURN1-8 switch 2-pin headers for off-board burn elements;
 *                firing one does nothing unless something is plugged in.
 */
#define GATE(port, pin) { DEVICE_DT_GET(DT_NODELABEL(port)), pin, GPIO_ACTIVE_HIGH }
static const struct gpio_dt_spec gates[] = {
    GATE(gpiob, 1), GATE(gpioc, 10), GATE(gpioc, 4), GATE(gpiob, 4),
    GATE(gpiob, 0), GATE(gpioc, 11), GATE(gpioc, 5), GATE(gpioc, 12),
    GATE(gpioa, 12), GATE(gpioa, 11), GATE(gpioc, 8), GATE(gpioc, 7),
    GATE(gpiob, 7), GATE(gpiob, 6), GATE(gpioa, 2), GATE(gpioc, 3),
};
static const char *const gate_names[] = {
    "PB1  BURN1    Q1  R14A (antenna)",
    "PC10 BURN2    Q2  R15A (antenna)",
    "PC4  BURN3    Q3  R16A (antenna)",
    "PB4  BURN4    Q4  R17A (antenna)",
    "PB0  BURN5    Q5  R14B (antenna)",
    "PC11 BURN6    Q6  R15B (antenna)",
    "PC5  BURN7    Q7  R16B (antenna)",
    "PC12 BURN8    Q8  R17B (antenna)",
    "PA12 SP_BURN1 Q9  hdr SP2 (solar)",
    "PA11 SP_BURN2 Q10 hdr SP1 (solar)",
    "PC8  SP_BURN3 Q11 hdr P4 (solar)",
    "PC7  SP_BURN4 Q12 hdr P5 (solar)",
    "PB7  SP_BURN5 Q13 hdr SP4 (solar)",
    "PB6  SP_BURN6 Q14 hdr SP3 (solar)",
    "PA2  SP_BURN7 Q15 hdr SP6 (solar)",
    "PC3  SP_BURN8 Q16 hdr SP5 (solar)",
};
BUILD_ASSERT(ARRAY_SIZE(gates) == ARRAY_SIZE(gate_names));

static void all_off(void)
{
    for (size_t i = 0; i < ARRAY_SIZE(gates); ++i) {
        gpio_pin_set_dt(&gates[i], 0);
    }
    gpio_pin_set_dt(&led, 0);
}

static void list_gates(void)
{
    for (size_t i = 0; i < ARRAY_SIZE(gates); ++i) {
        printk("  %2u: %s\n", (unsigned)i, gate_names[i]);
    }
}

static void fire(unsigned n, unsigned ms)
{
    unsigned char c;

    while (rx_get(&c) == 0) {
        /* Drain input so the abort check below only sees new bytes. */
    }
    printk("FIRE gate %u (%s) for %u ms -- press any key to abort\n",
           n, gate_names[n], ms);

    all_off();
    gpio_pin_set_dt(&gates[n], 1);
    gpio_pin_set_dt(&led, 1);

    int64_t start = k_uptime_get();
    bool aborted = false;
    while (k_uptime_get() - start < ms) {
        if (rx_get(&c) == 0) {
            aborted = true;
            break;
        }
        k_msleep(5);
    }
    int64_t elapsed = k_uptime_get() - start;

    all_off();
    printk("%s: gate %u was on for %lld ms; all gates low\n",
           aborted ? "ABORTED" : "DONE", n, (long long)elapsed);
}

static void handle_line(char *line)
{
    char *cmd = strtok(line, " \t");

    if (cmd == NULL) {
        return;
    }
    if (strcmp(cmd, "l") == 0) {
        list_gates();
    } else if (strcmp(cmd, "x") == 0) {
        all_off();
        printk("all gates low\n");
    } else if (strcmp(cmd, "f") == 0) {
        char *n_str = strtok(NULL, " \t");
        char *ms_str = strtok(NULL, " \t");
        char *end;

        if (n_str == NULL) {
            printk("usage: f <gate 0-15> [ms]\n");
            return;
        }
        unsigned long n = strtoul(n_str, &end, 10);
        if (*end != '\0' || n >= ARRAY_SIZE(gates)) {
            printk("bad gate '%s' (0-%u)\n", n_str, (unsigned)ARRAY_SIZE(gates) - 1);
            return;
        }
        unsigned long ms = BURN_DEFAULT_MS;
        if (ms_str != NULL) {
            ms = strtoul(ms_str, &end, 10);
            if (*end != '\0' || ms == 0 || ms > BURN_MAX_MS) {
                printk("bad duration '%s' (1-%u ms)\n", ms_str, BURN_MAX_MS);
                return;
            }
        }
        fire((unsigned)n, (unsigned)ms);
    } else {
        printk("unknown command '%s' (l, f <n> [ms], x)\n", cmd);
    }
}

int main(void)
{
    for (size_t i = 0; i < ARRAY_SIZE(gates); ++i) {
        if (!gpio_is_ready_dt(&gates[i]) ||
            gpio_pin_configure_dt(&gates[i], GPIO_OUTPUT_INACTIVE) != 0) {
            printk("DANT burn test: failed to hold gate %u low\n", (unsigned)i);
            return 0;
        }
    }
    if (!gpio_is_ready_dt(&led) ||
        gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE) != 0 ||
        !device_is_ready(uart)) {
        printk("DANT burn test: LED/UART setup failed\n");
        return 0;
    }
    if (uart_irq_callback_user_data_set(uart, uart_rx_isr, NULL) != 0) {
        printk("DANT burn test: UART RX interrupt setup failed\n");
        return 0;
    }
    uart_irq_rx_enable(uart);

    printk("\nDANT REV1 burn wire test: all gates low\n");
    printk("commands: l | f <n> [ms] (default %u, max %u) | x\n",
           BURN_DEFAULT_MS, BURN_MAX_MS);
    list_gates();
    printk("> ");

    char line[32];
    size_t len = 0;
    while (1) {
        unsigned char c;

        if (rx_get(&c) != 0) {
            k_msleep(10);
            continue;
        }
        if (c == '\r' || c == '\n') {
            printk("\n");
            line[len] = '\0';
            handle_line(line);
            len = 0;
            printk("> ");
        } else if ((c == '\b' || c == 0x7f) && len > 0) {
            --len;
            printk("\b \b");
        } else if (c >= ' ' && len < sizeof(line) - 1) {
            line[len++] = (char)c;
            printk("%c", c);
        }
    }
}

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>

#include <RadioLib.h>
#include <STMRadioHal.h>
#include <zephyr/drivers/spi.h>
#include <string.h>
#include "radio.h"

#define SPI_DEV    DT_NODELABEL(spi1)
#define GPIO_NODE  DT_NODELABEL(gpioa)
#define GPIO_CS_NODE DT_NODELABEL(gpioa)
#define CS_PIN     4
#define IRQ_PIN    8
#define RST_PIN    9
#define BUSY_PIN   10
#define RADIO_TASK_STACK_SIZE 4096
#define RADIO_TASK_PRIORITY 5

enum class State {
    BOOT,
    WAKE,
    DEPLOYING,
    VITALS,
    REGULAR,
    FAULT,
    RESTART,
    SAFE,
    RADIO_TEST
};

LOG_MODULE_REGISTER(radio_task, CONFIG_LOG_DEFAULT_LEVEL);

const struct device *const spi_bus = DEVICE_DT_GET(SPI_DEV);
const struct device *const gpio_port = DEVICE_DT_GET(GPIO_NODE);
const struct device *const gpio_cs_port = DEVICE_DT_GET(GPIO_CS_NODE);

K_THREAD_STACK_DEFINE(radio_task_stack, RADIO_TASK_STACK_SIZE);
static struct k_thread radio_task_thread;
static bool radio_thread_started = false;

volatile bool tx_done = false;
State currentState = State::BOOT;

void onTxDone(void)
{
    tx_done = true;
}

static void log_vitals_snapshot(void)
{
    const uint8_t active_radio = radio_which();
    const char *active_radio_name = (active_radio == 0U) ? "SX1268" : "RFM98";

    LOG_INF("Vitals snapshot: uptime=%u ms", k_uptime_get_32());
    LOG_INF("Radio: active=%s (%u) sx_state=%d rfm_state=%d queue_used=%u queue_free=%u",
            active_radio_name,
            active_radio,
            radio_get_SX_state(),
            radio_get_RFM_state(),
            k_msgq_num_used_get(&radio_msgq),
            k_msgq_num_free_get(&radio_msgq));
    LOG_INF("Devices: spi_ready=%d gpio_ready=%d gpio_cs_ready=%d",
            device_is_ready(spi_bus),
            device_is_ready(gpio_port),
            device_is_ready(gpio_cs_port));

#if IS_ENABLED(CONFIG_SENSOR)
    LOG_INF("Sensors: CONFIG_SENSOR enabled, but no sensor devices are registered in DANT app yet");
#else
    LOG_INF("Sensors: CONFIG_SENSOR not enabled; no additional sensor vitals available");
#endif
}

static void radio_queue_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    radio_task_cpp();
}

static bool start_radio_thread_once(void)
{
    if (radio_thread_started) {
        return true;
    }

    k_thread_create(
        &radio_task_thread,
        radio_task_stack,
        K_THREAD_STACK_SIZEOF(radio_task_stack),
        radio_queue_entry,
        NULL,
        NULL,
        NULL,
        RADIO_TASK_PRIORITY,
        0,
        K_NO_WAIT);

    radio_thread_started = true;
    return true;
}

int main(void)
{
    while (1) {
        switch (currentState) {
        case State::BOOT: {
            bool ready = true;

            if (!device_is_ready(spi_bus)) {
                LOG_ERR("SPI bus not ready");
                ready = false;
            }

            if (!device_is_ready(gpio_port)) {
                LOG_ERR("GPIO port not ready");
                ready = false;
            }

            if (!device_is_ready(gpio_cs_port)) {
                LOG_ERR("GPIO CS port not ready");
                ready = false;
            }

            if (!ready) {
                currentState = State::FAULT;
                break;
            }

            if (init_radio() != 0) {
                LOG_ERR("Radio init failed");
                currentState = State::FAULT;
                break;
            }

            if (!start_radio_thread_once()) {
                LOG_ERR("Radio thread start failed");
                currentState = State::FAULT;
                break;
            }

            currentState = State::WAKE;
            break;
        }

        case State::WAKE:
            LOG_INF("State: WAKE");
            currentState = State::VITALS;
            break;

        case State::DEPLOYING:
            LOG_INF("State: DEPLOYING");
            currentState = State::REGULAR;
            break;

        case State::VITALS:
            LOG_INF("State: VITALS");
            log_vitals_snapshot();
            currentState = State::REGULAR;
            break;

        case State::REGULAR:
            // Normal operations placeholder.
            k_msleep(100);
            break;

        case State::SAFE:
            LOG_WRN("State: SAFE");
            k_msleep(250);
            break;

        case State::RADIO_TEST:
            LOG_INF("State: RADIO_TEST");
            radio_test();
            currentState = State::REGULAR;
            break;

        case State::FAULT:
            LOG_ERR("State: FAULT");
            currentState = State::SAFE;
            break;

        case State::RESTART:
            LOG_WRN("State: RESTART");
            k_msleep(100);
            sys_reboot(SYS_REBOOT_COLD); // this is powering off and restarting, not just rebooting
            break;
        }
    }

    return 0;
}

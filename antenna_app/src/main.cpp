/**
 * @file main.cpp
 * @brief Application entry point and top-level state machine.
 *
 * Startup initializes metrics, persists the boot count, checks core devices,
 * starts the radio worker, and advances through the application states.
 * Recoverable startup failures increment the metrics fault counter before the
 * state machine enters FAULT/RESTART.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>

#include <RadioLib.h>
#include <STMRadioHal.h>
#include <zephyr/drivers/spi.h>
#include <string.h>
#include "metrics.h"
#include "radio.h"

#define SPI_DEV DT_NODELABEL(spi1)
#define GPIO_NODE DT_NODELABEL(gpioa)
#define GPIO_CS_NODE DT_NODELABEL(gpioa)
#define CS_PIN 4
#define IRQ_PIN 8
#define RST_PIN 9
#define BUSY_PIN 10
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

/**
 * @brief RadioLib packet-sent callback used by manual TX experiments.
 */
void onTxDone(void)
{
    tx_done = true;
}

/**
 * @brief Zephyr thread entry wrapper for the C++ radio task.
 *
 * @param p1 Unused.
 * @param p2 Unused.
 * @param p3 Unused.
 */
static void radio_queue_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    radio_task_cpp();
}

/**
 * @brief Start the radio worker thread once.
 *
 * @retval true if the thread is already running or was started.
 */
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

/**
 * @brief Initialize application services and run the state machine.
 *
 * @return 0 if the state machine exits.
 */
int main(void)
{
    metrics_init();
    metrics_inc_boot();

    while (1) {
        switch (currentState) {
        case State::BOOT: {
            bool ready = true;

            if (!device_is_ready(spi_bus)) {
                metrics_inc_fault();
                LOG_ERR("SPI bus not ready");
                ready = false;
            }

            if (!device_is_ready(gpio_port)) {
                metrics_inc_fault();
                LOG_ERR("GPIO port not ready");
                ready = false;
            }

            if (!device_is_ready(gpio_cs_port)) {
                metrics_inc_fault();
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
                metrics_inc_fault();
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
            currentState = State::REGULAR;
            break;

        case State::REGULAR:
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
            currentState = State::RESTART;
            break;

        case State::RESTART:
            LOG_WRN("State: RESTART");
            k_msleep(100);
            sys_reboot(SYS_REBOOT_COLD);
            break;
        }
    }

    return 0;
}

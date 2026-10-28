#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "radio.h"

#define RADIO_TASK_STACK_SIZE 4096
#define RADIO_TASK_PRIORITY 5

LOG_MODULE_REGISTER(radio_task, CONFIG_LOG_DEFAULT_LEVEL);

K_THREAD_STACK_DEFINE(radio_task_stack, RADIO_TASK_STACK_SIZE);
static struct k_thread radio_task_thread;

static void radio_queue_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    radio_task_cpp();
}

/**
 * @brief Initialize the radio and run the packet producer.
 * @return Zero if initialization fails; otherwise does not return.
 */
int main(void)
{
    /* init_radio() checks the GPIO/SPI devices and initializes the radio. */
    const int result = init_radio();
    if (result != 0) {
        LOG_ERR("Radio initialization failed: %d; test stopped", result);
        return 0;
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

    /* Produce test packets; the worker handles queued radio operations. */
    radio_test();
    return 0;
}

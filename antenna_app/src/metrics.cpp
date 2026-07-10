/**
 * @file metrics.cpp
 * @brief Metrics counter storage and update routines.
 *
 * Runtime counters are stored as Zephyr atomics so they can be updated from
 * the main thread, radio task, or callback paths. The boot counter is mirrored
 * in RAM but persisted through the Zephyr Settings API using the NVS backend.
 */

#include "metrics.h"

#include <errno.h>
#include <stddef.h>
#include <sys/types.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/atomic.h>

#define METRICS_BOOT_COUNT_KEY "metrics/boot_count"

static atomic_t boot_count;
static atomic_t fault_count;
static atomic_t radio_tx_attempts;
static atomic_t radio_tx_successes;
static atomic_t radio_tx_failures;
static atomic_t radio_retries;
static atomic_t radio_queue_full_count;
static atomic_t successful_cycles;

static bool persistent_metrics_ready;

/**
 * @brief Convert a Zephyr atomic counter to the public 32-bit metric type.
 *
 * @param counter Atomic counter to read.
 * @return Current counter value.
 */
static uint32_t metrics_counter_get(const atomic_t *counter)
{
    return (uint32_t)atomic_get(counter);
}

/**
 * @brief Load the persisted boot count from Settings/NVS.
 *
 * A missing key is treated as a first boot and returns zero.
 *
 * @param value Output location for the loaded boot count.
 * @retval 0 if a value was loaded or the key did not exist.
 * @retval -EINVAL if the stored value length is unexpected.
 * @retval negative Zephyr error code from settings_load_one().
 */
static int metrics_load_boot_count(uint32_t *value)
{
    ssize_t ret = settings_load_one(METRICS_BOOT_COUNT_KEY, value, sizeof(*value));

    if (ret == -ENOENT) {
        *value = 0U;
        return 0;
    }

    if (ret < 0) {
        return (int)ret;
    }

    return ret == sizeof(*value) ? 0 : -EINVAL;
}

/**
 * @brief Persist a boot count value to Settings/NVS.
 *
 * @param value Boot count to save.
 * @retval 0 on success.
 * @retval negative Zephyr error code from settings_save_one().
 */
static int metrics_save_boot_count(uint32_t value)
{
    return settings_save_one(METRICS_BOOT_COUNT_KEY, &value, sizeof(value));
}

void metrics_init(void)
{
    uint32_t stored_boot_count = 0U;
    int ret = settings_subsys_init();

    if (ret != 0) {
        persistent_metrics_ready = false;
        atomic_inc(&fault_count);
        return;
    }

    persistent_metrics_ready = true;

    ret = metrics_load_boot_count(&stored_boot_count);
    if (ret != 0) {
        atomic_inc(&fault_count);
        return;
    }

    atomic_set(&boot_count, (atomic_val_t)stored_boot_count);
}

void metrics_inc_boot(void)
{
    uint32_t value = (uint32_t)atomic_inc(&boot_count) + 1U;

    if (persistent_metrics_ready && metrics_save_boot_count(value) != 0) {
        atomic_inc(&fault_count);
    }
}

int metrics_reset_boot_count(void)
{
    uint32_t value = 0U;

    if (!persistent_metrics_ready) {
        return -EIO;
    }

    int ret = metrics_save_boot_count(value);
    if (ret != 0) {
        return ret;
    }

    atomic_set(&boot_count, (atomic_val_t)value);
    return 0;
}

void metrics_inc_fault(void)
{
    atomic_inc(&fault_count);
}

void metrics_inc_radio_tx_attempt(void)
{
    atomic_inc(&radio_tx_attempts);
}

void metrics_inc_radio_tx_success(void)
{
    atomic_inc(&radio_tx_successes);
}

void metrics_inc_radio_tx_failure(void)
{
    atomic_inc(&radio_tx_failures);
}

void metrics_inc_radio_retry(void)
{
    atomic_inc(&radio_retries);
}

void metrics_inc_radio_queue_full(void)
{
    atomic_inc(&radio_queue_full_count);
}

void metrics_inc_successful_cycle(void)
{
    atomic_inc(&successful_cycles);
}

struct metrics_snapshot metrics_get_snapshot(void)
{
    struct metrics_snapshot snapshot = {};

    snapshot.boot_count = metrics_counter_get(&boot_count);
    snapshot.fault_count = metrics_counter_get(&fault_count);
    snapshot.radio_tx_attempts = metrics_counter_get(&radio_tx_attempts);
    snapshot.radio_tx_successes = metrics_counter_get(&radio_tx_successes);
    snapshot.radio_tx_failures = metrics_counter_get(&radio_tx_failures);
    snapshot.radio_retries = metrics_counter_get(&radio_retries);
    snapshot.radio_queue_full_count = metrics_counter_get(&radio_queue_full_count);
    snapshot.successful_cycles = metrics_counter_get(&successful_cycles);

    return snapshot;
}

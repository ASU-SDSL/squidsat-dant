/**
 * @file metrics.h
 * @brief Runtime and persistent health metrics for the antenna application.
 *
 * The metrics module tracks core health counters that can be sampled by
 * telemetry, debug commands, or future CAN/protobuf status messages. Most
 * counters are runtime-only atomics. The boot counter is backed by Zephyr
 * Settings/NVS so it survives reboot and power loss.
 */

#ifndef METRICS_H
#define METRICS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Snapshot of all metrics counters.
 *
 * A snapshot is a point-in-time copy. It does not lock future updates, so use
 * it as telemetry/debug state rather than as a transactional record.
 */
struct metrics_snapshot {
    /** Number of completed boot-count increments persisted in Settings/NVS. */
    uint32_t boot_count;
    /** Count of recoverable fault/error paths observed since this boot. */
    uint32_t fault_count;
    /** Number of radio transmit operations attempted since this boot. */
    uint32_t radio_tx_attempts;
    /** Number of radio transmit operations that completed successfully. */
    uint32_t radio_tx_successes;
    /** Number of radio transmit operations that failed after retries. */
    uint32_t radio_tx_failures;
    /** Number of additional radio transmit attempts after first failure. */
    uint32_t radio_retries;
    /** Number of radio queue insertions rejected because the queue was full. */
    uint32_t radio_queue_full_count;
    /** Number of application cycles that reached a successful TX completion. */
    uint32_t successful_cycles;
};

/**
 * @brief Initialize metrics storage and load persistent counters.
 *
 * This initializes the Zephyr Settings backend and loads the persisted
 * `boot_count`. If persistent storage cannot be initialized or read, the
 * module keeps running with runtime counters and records a fault.
 */
void metrics_init(void);

/**
 * @brief Increment and persist the boot counter.
 *
 * Call once during application startup after metrics_init(). The saved value is
 * updated immediately so the count survives a later reset or power loss.
 */
void metrics_inc_boot(void);

/**
 * @brief Reset the persistent and in-memory boot count to zero.
 *
 * The next startup call to metrics_inc_boot() will report boot_count == 1.
 *
 * @retval 0 if the reset was saved and applied.
 * @retval -EIO if persistent metrics storage is not ready.
 * @retval negative Zephyr error code if Settings/NVS could not save the value.
 */
int metrics_reset_boot_count(void);

/** @brief Increment the recoverable fault counter. */
void metrics_inc_fault(void);

/** @brief Increment the radio transmit attempt counter. */
void metrics_inc_radio_tx_attempt(void);

/** @brief Increment the radio transmit success counter. */
void metrics_inc_radio_tx_success(void);

/** @brief Increment the radio transmit failure counter. */
void metrics_inc_radio_tx_failure(void);

/** @brief Increment the radio retry counter. */
void metrics_inc_radio_retry(void);

/** @brief Increment the radio queue-full counter. */
void metrics_inc_radio_queue_full(void);

/** @brief Increment the successful application cycle counter. */
void metrics_inc_successful_cycle(void);

/**
 * @brief Return a point-in-time copy of all metrics counters.
 *
 * @return Snapshot containing persistent boot count and runtime counters.
 */
struct metrics_snapshot metrics_get_snapshot(void);

#ifdef __cplusplus
}
#endif

#endif /* METRICS_H */

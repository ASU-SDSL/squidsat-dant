# Metrics Tracking Summary

## Purpose

This change adds a small health metrics layer for the antenna application. It tracks boot persistence, fault paths, radio transmit behavior, queue pressure, and successful application cycles.

## Metrics Added

| Metric | Persistence | Meaning |
| --- | --- | --- |
| `boot_count` | Settings/NVS | Number of boots recorded across resets and power cycles. |
| `fault_count` | Runtime | Recoverable errors observed during this boot. |
| `radio_tx_attempts` | Runtime | Queued or manual radio transmit operations attempted. |
| `radio_tx_successes` | Runtime | Radio transmit operations that completed successfully. |
| `radio_tx_failures` | Runtime | Radio transmit operations that failed after retries or setup checks. |
| `radio_retries` | Runtime | Additional radio transmit attempts after an initial failure. |
| `radio_queue_full_count` | Runtime | Radio queue insertions rejected by `k_msgq_put()`. |
| `successful_cycles` | Runtime | Application cycles that reached successful transmit completion. |

## File Structure

```text
antenna_app/src/metrics.h      Public metrics API and snapshot type
antenna_app/src/metrics.cpp    Atomic counters and persistent boot-count storage
antenna_app/src/main.cpp       Startup boot/fault metrics and manual TX metrics
antenna_app/src/radio.cpp      Radio queue, retry, TX, and successful-cycle metrics
antenna_app/src/can_handler.cpp CAN fault-path metrics
antenna_app/prj.conf           Settings/NVS/flash configuration
antenna_app/CMakeLists.txt     Builds metrics.cpp into the Zephyr app
```

## Persistence Model

Only `boot_count` is persistent. It is saved with Zephyr Settings using the NVS backend under:

```text
metrics/boot_count
```

The Nucleo-F103RB board DTS already provides a `storage_partition` at the end of flash, so no custom application overlay partition is required.

Runtime counters are intentionally not persisted to avoid frequent flash writes and flash wear.

## Reset Behavior

`metrics_reset_boot_count()` sets the persisted and in-memory boot count to `0`.

Expected behavior:

- Calling `metrics_reset_boot_count()` returns `0` after the reset is saved.
- The next call to `metrics_inc_boot()` makes `boot_count == 1`.
- If Settings/NVS is not ready, the function returns `-EIO`.
- If the flash write fails, the function returns the Zephyr error code and does not change the in-memory value.

## Integration Points

- `main()` calls `metrics_init()` and `metrics_inc_boot()` once at startup.
- Startup hardware readiness failures increment `fault_count`.
- Radio queued transmit operations increment `radio_tx_attempts`.
- Successful radio transmit completion increments `radio_tx_successes` and `successful_cycles`.
- Failed radio transmit completion increments `radio_tx_failures` and `fault_count`.
- Retry attempts inside `transmit_packet()` increment `radio_retries`.
- Failed radio queue insertion increments `radio_queue_full_count` and `fault_count`.
- CAN setup/send failures increment `fault_count`.

## Configuration Added

```conf
CONFIG_FLASH=y
CONFIG_FLASH_MAP=y
CONFIG_SETTINGS=y
CONFIG_NVS=y
CONFIG_SETTINGS_NVS=y
CONFIG_MPU_ALLOW_FLASH_WRITE=y
```

## Verification Notes

The touched C++ files compiled directly using the generated compile commands:

```text
metrics.cpp
radio.cpp
main.cpp
can_handler.cpp
```

`west build` is currently blocked before full application linking because Nanopb generation cannot find `protoc` on `PATH`.

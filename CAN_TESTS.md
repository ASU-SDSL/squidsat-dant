# CAN tests

These targets run on two Nucleo-F103RB boards with external CAN transceivers.
Use PA12/TX, PA11/RX, common ground, and a terminated bus at 50 kbit/s.
These pin assignments are **not for the DANT PCB**, where PA11/PA12 drive deployment gates.
The tests do not use an application state machine or initialize a radio.

## Raw CAN smoke test

```sh
west build -p always -b nucleo_f103rb can_smoke_test -d build-can-a -- '-DCONF_FILE=prj.conf;node_a.conf'
west build -p always -b nucleo_f103rb can_smoke_test -d build-can-b -- '-DCONF_FILE=prj.conf;node_b.conf'
```

Node A sends standard ID 0x123, byte 42, every second. Node B prints received
frames at 115200 baud on the Nucleo virtual COM port. `CONFIG_DANT_TX_PERIOD_MS`
sets the send interval. The worker in `antenna_app/src/can_handler.cpp` starts
automatically; its old init/send declarations have been removed.

## ISO-TP payload test

This is the validated transport test, independent of `can_handler.cpp`.
Initialize the pinned module with `git submodule update --init sdsl-can`.
Run from the Zephyr workspace root:

```sh
west build -p always -b nucleo_f103rb sdsl-can/sdsl-can-lib/tests/payload_size_test -d build-isotp-duplex-a -- -DZEPHYR_EXTRA_MODULES="$PWD/sdsl-can/sdsl-can-lib" '-DCONF_FILE=prj.conf;node_a.conf;duplex.conf'
west build -p always -b nucleo_f103rb sdsl-can/sdsl-can-lib/tests/payload_size_test -d build-isotp-duplex-b -- -DZEPHYR_EXTRA_MODULES="$PWD/sdsl-can/sdsl-can-lib" '-DCONF_FILE=prj.conf;node_b.conf;duplex.conf'
```

Omit `duplex.conf` for one-way traffic. The test compares 32, 64, and 128 encoded
Protobuf bytes, routing metadata, and sequence numbers. Require matching RX PASS
records and completion logs in both directions, without gaps, duplicates or CAN errors.
A successful send call alone does not prove delivery. The C++ adapter in
`antenna_app/src/can_interface.h` wraps this module; broadcasts are limited to 7 bytes.

[Detailed criteria and previous hardware results](sdsl-can/sdsl-can-lib/tests/payload_size_test/README.md)
record the 2026-10-14 one-way and duplex checks. They cover a short, two-node,
50 kbit/s test, not 500 kbit/s, broadcast, endurance, or fault recovery.
The module stays pinned at `b984aaa6102870eda2f1b079e261b3e2d340ca81`.

The radio target remains under `antenna_app`; use these standalone CAN targets
when testing CAN alone. Both feature branches share the state-machine removal
commit so they can be merged into main in either order.

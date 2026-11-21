# DANT REV1 burn wire test: setup guide

Follow this top to bottom to flash the burn-wire test firmware onto a DANT REV1
board and fire the burn resistors (R14A and R14B first). For the detailed
meter checks and expected readings, see `R14_TEST_PROCEDURE.md` in this folder.

> **Safety.** The burn resistors reach very high temperatures (about 3-5 W in a
> 0805 part). Wear eye protection, keep fingers off the board during a burn,
> and never leave a burn running unattended. The firmware caps a burn at 10 s
> and any keypress aborts it. Do not flash a Nucleo image onto the DANT board.

## What you need

- DANT REV1 board with these parts fitted: U1 (STM32F103), U7 (LT3083),
  R30 (20k), R31 (0.625 ohm), Q1/Q5, R14A/R14B, R2/R6. The radio is not needed.
- ST-Link V3 (or V2) debug probe and a USB cable.
- Bench supply, 5.0 V, current limit **2.5 A**, for the burn power.
- 3.3 V supply (100 mA) for the MCU, or an equivalent 3.3 V source.
- Short, thick leads (18-20 AWG, under 30 cm) for the 5 V and ground. Thin
  wires drop too much voltage and the burn will come out weak.
- Multimeter (ideally with MIN/MAX), a jumper, and eye protection.
- Nylon burn line (0.2-0.3 mm works best) for the final burn test.

## 1. Install the tools (one time)

macOS (Homebrew):

```bash
brew install cmake ninja stlink openocd python
```

Download and install the Zephyr SDK (ARM toolchain), following
https://docs.zephyrproject.org/latest/develop/getting_started/index.html .
Typical install location: `~/zephyr-sdk`.

Clone the repo and set up the Python environment and the Zephyr workspace:

```bash
git clone <repo-url> squidsat-dant
```

```bash
cd squidsat-dant && git submodule update --init --recursive
```

```bash
python3 -m venv .venv && source .venv/bin/activate
```

```bash
python -m pip install --upgrade pip west
```

```bash
west init -l antenna_app && west update && west zephyr-export
```

```bash
python -m pip install -r zephyr/scripts/requirements.txt
```

If a `west` workspace and `.venv` already exist in your checkout, skip the
`west init` and `west update` steps.

## 2. Wire the board

All connections go to the board's headers. Positions are from the PcbDoc.

| Connection | Board point | Notes |
|---|---|---|
| +5V (from bench supply) | `J1` pin A2 | main burn power |
| +3.3V | `J1` pin A1 (or `P1` pin 11) | powers the MCU |
| Ground | `J1` pins 3 and 4 (or `P1` pin 1) | use both `J1` ground pins |
| BOOT0 jumper | `J2` pins 2-3 | forces boot from flash. Without it BOOT0 floats |

ST-Link to `P1` (the 12-pin header). Pin 1 is GND:

| ST-Link signal | `P1` pin |
|---|---|
| GND | 1 |
| SWCLK | 3 |
| SWDIO | 7 |
| NRST | 9 (strongly recommended) |

Optional: `P1` pin 2 is +5V and pin 11 is +3.3V, but do not feed the same rail
from two sources.

**Serial console:** the firmware prints on USART1 (PA9 = `P1` pin 10 is the
board's TX, PA10 = `P1` pin 8 is its RX). The simplest option is the ST-Link
V3's built-in USB serial port if your ST-Link breakout wires those pins; that
is how it was tested. Otherwise use a 3.3 V USB-UART adapter: adapter RX to
`P1` pin 10, adapter TX to `P1` pin 8, GND to `P1` pin 1. 115200 8N1.

## 3. Unpowered checks

Multimeter on continuity / resistance, board **not** powered.

| Measure | Expect |
|---|---|
| `J1` pin 3, pin 4, `J2` pin 3, `P1` pin 1 | all beep together (GND) |
| `5V` test point to GND | open (over 1 kohm) |
| TP10 to GND | open |
| R31 pad to pad | about 0.6 ohm |
| R14A pad to pad, R14B pad to pad | about 2 ohm each |
| R14A pad 2 to GND, R14B pad 2 to GND | open |
| Q1 gate to GND, Q5 gate to GND | about 10 kohm |

If any check fails, stop and fix the board before powering it.

## 4. Power up with the gates off

1. Fit the jumper on `J2` pins 2-3.
2. Apply 3.3 V, then 5 V.
3. Check with the meter, to GND:
   - `5V` test point: about 5 V
   - `3.3V` test point: about 3.3 V
   - TP10 (beside U7, top side): steady, roughly 3.5-5 V
   - across R31, R14A and R14B: 0 V

## 5. Connect to the ST-Link V3 and flash

Check that the probe sees the board. This prints the ST-Link version and the
chip. If it says `Failed to enter SWD mode`, the board has no 3.3 V or the SWD
wires are wrong:

```bash
st-info --probe
```

Build the firmware from the workspace root (activate the venv first:
`source .venv/bin/activate`):

```bash
export ZEPHYR_SDK_INSTALL_DIR=$HOME/zephyr-sdk
```

```bash
west build -p always -b nucleo_f103rb burn_wire_test
```

The board name is only used for the SoC definition; the overlay in
`burn_wire_test/boards/` re-maps everything for the DANT board.

Flash with connect-under-reset, which works even if a previous image
disabled the debug pins:

```bash
st-flash --connect-under-reset write build/zephyr/zephyr.bin 0x08000000
```

Success ends with `Flash written and verified!`.

If the flash fails with USB timeouts, unplug the ST-Link, wait a few seconds,
plug it in again (direct port, not a hub) and retry. Close any other program
holding the probe (STM32CubeIDE, STM32CubeProgrammer, a running `openocd`).

OpenOCD alternative:

```bash
openocd -f interface/stlink.cfg -c "transport select hla_swd" -f target/stm32f1x.cfg -c "program build/zephyr/zephyr.bin 0x08000000 verify reset exit"
```

## 6. Open the serial terminal

Find the port. The ST-Link V3 shows up as a USB modem device:

```bash
ls /dev/cu.usbmodem*
```

Connect at 115200 baud (replace the port with yours):

```bash
screen /dev/cu.usbmodem102 115200
```

Press Enter. You should see the banner `DANT REV1 burn wire test: all gates low`
and the `>` prompt. To quit `screen`: Ctrl-A, then K, then Y. Quit it before
flashing again, or the port stays busy.

Linux uses `/dev/ttyACM0` instead of `/dev/cu.usbmodem*`. On Windows use the
COM port shown in Device Manager with PuTTY or Tera Term.

## 7. Firmware commands

Type the command and press Enter.

| Command | Action |
|---|---|
| `l` | list the 16 gates |
| `f <n> [ms]` | fire gate n for ms milliseconds (default 2000, max 10000) |
| `x` | force all gates off |

Any key pressed during a burn aborts it. Only one gate fires at a time.

| Gate | MCU pin | Resistor |
|---|---|---|
| 0 | PB1 | R14A |
| 4 | PB0 | R14B |

The other gates drive the remaining antenna resistors (gates 1-3 and 5-7) and
the solar-panel headers (gates 8-15); run `l` for the full list.

## 8. First burn test

1. Meter on DC volts with MIN/MAX, across **R31** (0.625 ohm, beside U7).
2. Type `f 0 1000`.
3. Expect: D1 lights for 1 s, the console prints
   `DONE: gate 0 was on for 1000 ms; all gates low`, and the meter MAX reads
   about 0.8-1.0 V across R31 (volts / 0.625 = amps, about 1.3-1.6 A).
4. Move the meter across R14A and fire again: expect about 2.6-3.2 V.
5. Move the red lead to R14A pad 2 (the Q1 drain), black on GND, fire again:
   expect under 0.15 V during the burn.
6. Repeat with `f 4 1000` for R14B.

Watch the 5 V rail at the board (`5V` test point) during a pulse. It must stay
above about 4.8 V. If it sags, the current comes out low (a sag to 3.8 V gave
only 1 A and a cold resistor). Fix it with shorter, thicker leads, both `J1`
ground pins, or the supply's sense terminals.

## 9. Burn the nylon

Only after step 8 passes.

1. Wrap the nylon two or three times around the body of R14A, snug, with a
   little tension (a small weight or the spring load).
2. Type `f 0 2000`. If it does not cut, let the board cool for 30 s and try
   `f 0 3000`, then `f 0 5000`.
3. After every burn, measure R14A pad to pad with the board unpowered. It
   should still read about 2 ohm. Replace the resistor if it is open, cracked
   or discoloured.
4. Repeat for R14B with `f 4 2000`.

Record the 5 V at the board, the volts across R31, the time to release and the
condition of the resistor afterwards.

## Troubleshooting

| Symptom | Likely cause |
|---|---|
| `st-info --probe` says `Failed to enter SWD mode` | no 3.3 V on the board, or SWDIO/SWCLK/GND wires wrong |
| USB timeouts from `st-flash` | ST-Link stuck: replug it, avoid hubs |
| No text in the terminal | wrong port, wrong baud (115200), or the board is not running. Press Enter |
| TP10 reads about 0 V with 5 V present | R31 missing (this happened on the first board), or U7 not working |
| Burn current only about 1 A, resistor stays cool | 5 V input sagging under load. Check the `5V` point during a pulse |
| Gate reads 3.3 V but no current | Q1/Q5 problem or the supply tripped its current limit |
| R14A reads open afterwards | the resistor burned out. Replace it and shorten the burn |

## Known REV1 board issues (do not block this test)

- R31 (0.625 ohm) is a 0603 carrying about 1.5 A (about 1.4 W). Keep burns to a
  few seconds, and fit a 1 W or larger part before long burns.
- CAN TX and RX look swapped between the MCU (PB8/PB9) and the TCAN330. CAN
  will not work until that is corrected.
- BOOT0 floats without the `J2` 2-3 jumper. VBAT (U1 pin 1) is unconnected.

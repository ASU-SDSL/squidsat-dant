# DANT REV1 burn resistor test: R14A and R14B

Bench procedure for the two antenna burn resistors on the BURN1 and BURN5
channels, using the `burn_wire_test` firmware and a multimeter. Net names,
pin numbers and positions come from `Squid DANT REV1.PcbDoc` and the REV1
schematic sheets (read with altium-monkey).

## Circuit under test

```
+5V (J1.A2) ──► U7 LT3083 IN/VCONTROL (pins 7-12)
                 U7 OUT (pins 1-5,13) ──► R31 0.625 Ω ──► "Regulator Output" (TP10)
                 U7 SET (pin 6) ──► R30 20k ──► "Regulator Output"
"Regulator Output" ──► R14A 2 Ω ──► Q1 drain ──► Q1 source ──► GND     gate 0, PB1
"Regulator Output" ──► R14B 2 Ω ──► Q5 drain ──► Q5 source ──► GND     gate 4, PB0
PB1 ──► Q1 gate, R2 10k to GND          PB0 ──► Q5 gate, R6 10k to GND
```

R30 is wired from SET to the far side of R31, not to ground. That makes the
LT3083 a **constant-current source**: it regulates the drop across R31 to
50 µA × 20 kΩ ≈ 1.0 V, i.e. about **1.6 A**, regardless of the burn resistor.
The VCONTROL headroom (needs about 1.2 V above OUT, and VCONTROL is only 5 V)
caps OUT near 3.6–3.8 V, so the real current will be roughly **1.3–1.6 A**.
Expect about **3–5 W in the 2 Ω resistor** while a gate is on.

With no gate on, no current flows, so the regulator cannot hold the 1 V drop
and its output rails up toward the 5 V input. **A TP10 reading close to the
5V test point at idle is correct**, not a fault.

## Equipment

- Bench supply, 5.0 V, current limit set to 2.5 A, for `+5V` (J1.A2).
- 3.3 V supply, 100 mA is plenty, for `+3.3V` (J1.A1). The MCU has no on-board
  regulator; without 3.3 V nothing drives the gates.
- ST-Link (or similar SWD probe) on P1, and a 3.3 V USB-UART adapter.
- Multimeter, ideally with MIN/MAX hold; the burn lasts 1–2 s.
- Jumper for J2 pins 2–3.
- Eye protection; the resistor and R31 get hot. Keep fingers off during a burn.

## Points to probe

Positions are PcbDoc coordinates in mils; use them to find the parts, then go
by the silkscreen.

| Point | Net | Side | Position | Notes |
|---|---|---|---|---|
| J1 A2 | +5V | bottom | (7761, 4116) | 5 V in |
| J1 A1 | +3.3V | bottom | (7761, 4263) | 3.3 V in |
| J1 3, 4 | GND | bottom | (7794, 4419), (7794, 4370) | ground reference |
| J2 pin 3 | GND | top | (6108, 2551) | alternate ground; pin 2 BOOT0, pin 1 3.3V |
| `5V` test point | +5V | bottom | (7616, 4113) | middle of the 5V RF / 5V / 3.3V column |
| `3.3V` test point | +3.3V | bottom | (7615, 4260) | |
| TP10 | Regulator Output | top | (7260, 5565) | beside U7 |
| R31 | OUT → Regulator Output | top | (7421, 5477) | 0.625 Ω: doubles as a current shunt |
| R14A | pad 1 Regulator Output, pad 2 Q1 drain | top | (4605, 5266) | gate 0 / PB1 |
| R14B | pad 1 Regulator Output, pad 2 Q5 drain | top | (4841, 5266) | gate 4 / PB0 |
| Q1 | G / D / S | top | (5042, 5005) | gate pulldown R2 (5120, 4928) |
| Q5 | G / D / S | top | (5140, 5104) | gate pulldown R6 (5216, 5024) |
| P1 | 12-pin SWD/UART header | top | (7092, 3624) | see below |

P1 pinout: 1 GND, 2 +5V, 3 SWCLK, 4 CAN_N, 5 SWO, 6 CAN_P, 7 SWDIO, 8 PA10
(MCU UART RX), 9 NRST, 10 PA9 (MCU UART TX), 11 +3.3V, 12 BOOT0.

## 1. Unpowered checks

Meter on continuity / Ω, nothing connected to the board.

| Measure | Expect | If not |
|---|---|---|
| J1.3 ↔ J1.4 ↔ J2.3 ↔ P1.1 | beep (all GND) | ground not stitched |
| `5V` test point ↔ GND | open, > 1 kΩ | 5 V rail shorted |
| TP10 ↔ GND | open, > 1 kΩ | burn rail shorted, suspect a shorted Q |
| TP10 ↔ R14A pad 1, TP10 ↔ R14B pad 1 | beep | Regulator Output trace broken |
| R14A pad 1 ↔ pad 2 | ≈ 2 Ω (meter lead resistance adds ~0.3 Ω) | resistor open or not fitted |
| R14B pad 1 ↔ pad 2 | ≈ 2 Ω | |
| R14A pad 2 ↔ GND | open (Q1 off) | Q1 shorted D–S |
| R14B pad 2 ↔ GND | open (Q5 off) | Q5 shorted D–S |
| Q1 gate ↔ GND, Q5 gate ↔ GND | ≈ 10 kΩ (R2, R6) | pulldown missing |
| `5V` ↔ `5V RF` test points | open or beep | only matters for the radio; either is fine for this test |

## 2. Power up, gates off

1. Fit the jumper on **J2 pins 2–3** (BOOT0 to GND). BOOT0 has only a series
   resistor to the header, so with no jumper it floats and the MCU may boot
   into the bootloader instead of flash.
2. Apply 3.3 V to J1.A1 first, then 5 V to J1.A2 (or via P1.11 and P1.2).
3. Supply current with nothing firing should be a few mA on 5 V and tens of mA
   on 3.3 V.

| Measure (to GND) | Expect |
|---|---|
| `5V` test point | 4.9–5.1 V |
| `3.3V` test point | 3.2–3.4 V |
| TP10 | roughly 3.5–5 V, steady. Close to the 5V rail is normal (see above). |
| R14A pad 2 and R14B pad 2 | same as TP10 (gates off, no drop across the resistor) |
| Across R14A, across R14B | 0 V |
| Across R31 | 0 V (no current) |
| Q1 gate, Q5 gate | 0 V |
| Supply current on 5 V | < 20 mA |

A TP10 reading of 0 V with 5 V present means U7 is not working (check pins
7–12 for 5 V, and that R30/R31 are fitted). A TP10 that is 0 V together with a
0 V `5V` test point means the board is not getting 5 V: check J1.A2 and the
supply.

## 3. Flash and connect the firmware

1. ST-Link on P1: SWDIO pin 7, SWCLK pin 3, GND pin 1, NRST pin 9 optional.
2. UART adapter: adapter RX → P1 pin 10 (PA9, MCU TX), adapter TX → P1 pin 8
   (PA10, MCU RX), GND → P1 pin 1. 115200 8N1.
3. Build and flash:

```bash
west build -b nucleo_f103rb burn_wire_test -p
```

```bash
west flash
```

4. Reset. The console prints `DANT REV1 burn wire test: all gates low` and the
   gate list. `l` lists gates; `x` forces all gates low. Gate **0** is R14A
   (PB1, Q1), gate **4** is R14B (PB0, Q5).

The firmware runs on the internal 8 MHz RC, so a bad crystal cannot stop it.
Any byte received while a gate is on aborts the burn, so do not type during
one unless you mean to stop it.

## 4. Electrical burn test, no burn wire

Run each channel one at a time. The regulator supplies ~1.6 A total, so two
gates at once just halves the current per resistor (the firmware prevents this
anyway). Start short.

### 4a. Gate check with a 1 s pulse

Meter on DC V, MIN/MAX enabled, red lead on **Q1 gate** (or R2 pad 1), black
on GND. Send:

```
f 0 1000
```

Expect MAX ≈ 3.3 V and D1 to light for 1 s. Console prints `DONE: gate 0 was
on for 1000 ms`. Repeat with the meter on Q5 gate and `f 4 1000`.

If the gate never rises: wrong pin / firmware not running. If D1 lights but the
gate stays at 0 V: Q gate or trace fault.

### 4b. Current through R14A

Meter across **R31** (both pads; polarity does not matter, note the sign),
MIN/MAX on. Send `f 0 1000`.

- MAX across R31 ≈ **0.8–1.0 V**. Current = V / 0.625 Ω, so **1.3–1.6 A**.
- Exactly ~1.0 V means the LT3083 is in current regulation; 0.8–0.9 V means
  it is headroom-limited by the 5 V VCONTROL. Both are fine.
- ~0 V: no current. Check Q1 gate (4a), then that the 5 V supply did not trip
  its current limit.
- Watch the bench supply: it should show about 1.5 A during the pulse and the
  `5V` rail should stay above ~4.75 V. If it sags a lot, the supply or leads are
  too weak and the current will be low.

### 4c. Voltages on the R14A channel

One quantity per pulse; repeat `f 0 1000` as needed.

| Red lead | Black lead | Expect during burn | Expect idle |
|---|---|---|---|
| R14A pad 1 | R14A pad 2 | 2.6–3.2 V (1.3–1.6 A × 2 Ω) | 0 V |
| R14A pad 2 (Q1 drain) | GND | < 0.15 V | = TP10 |
| TP10 | GND | 2.7–3.3 V | 3.5–5 V |
| R31 pad on U7 side | GND | 3.6–4.3 V | ≈ TP10 |
| Q1 gate | GND | 3.3 V | 0 V |

Drain voltage well above 0.2 V during a burn means Q1 is not fully on (gate
drive or a damaged FET). Power in the resistor ≈ V_R14A² / 2 Ω; at 3 V that is
4.5 W, which is what heats the wire.

### 4d. Same for R14B

Repeat 4b and 4c with `f 4 1000`, probing R14B, Q5 and R6.

### 4e. Thermal check of R31

R31 is fitted as a **0603**. At 1.5 A it dissipates about **1.4 W**, far above
a 0603's 0.1 W rating. It survives short pulses on thermal mass alone. After a
few 1 s pulses, let it cool and re-measure its resistance in-circuit
(~0.6 Ω). If it has drifted or discoloured, replace it with a 1 W or larger
0.625 Ω part before doing longer burns, and keep the burn under ~2 s until it is
replaced. This is the weakest part of the circuit.

## 5. Burn-wire test

Only after section 4 passes.

1. Wrap the nylon/Dyneema burn line once or twice around the body of R14A so
   it touches the resistor, with a light preload (a small weight or the antenna
   spring itself).
2. Nothing else touching the resistor. Camera on it if you want timing.
3. Send `f 0 2000` (the default). Note the time to release in the video or by
   eye. 2 s at 4–5 W is typical for 0.3–0.5 mm nylon; if it does not cut, let
   the board cool 30 s and try `f 0 3000`, then `f 0 5000`. Do not exceed 5 s
   until R31 is upgraded (4e).
4. Repeat for R14B with `f 4 2000`.
5. Record: supply voltage during burn, V across R31 (current), time to release,
   resistor condition afterwards (2 Ω, no cracks).

## Pass criteria

- Unpowered: no shorts on 5V or Regulator Output, ~2 Ω across each R14x,
  ~10 kΩ gate to GND.
- Powered idle: 5V 4.9–5.1 V, TP10 steady, 0 V across R14A/B and R31, gates 0 V.
- Burn: gate 3.3 V, 0.8–1.0 V across R31 (1.3–1.6 A), 2.6–3.2 V across the
  resistor, drain < 0.15 V, 5V rail holds, resistor measures 2 Ω afterwards.
- Burn wire releases within the commanded time.

## Design notes found while checking the board (not blocking this test)

- **CAN TX/RX appear swapped.** MCU PB8 (pin 61, CAN_RX with remap) connects to
  TCAN330 pin 1 TXD, and PB9 (pin 62, CAN_TX) to TCAN330 pin 4 RXD. The MCU's
  CAN pins cannot be swapped in software, so CAN will not work without
  cutting and crossing those two traces. Confirm on the schematic before
  modifying anything.
- **BOOT0 floats** without a J2 jumper (only R11 in series to the header).
- **VBAT (U1 pin 1) is unconnected.** ST recommends tying it to VDD when no
  backup battery is used. Usually harmless.
- **R31 (0.625 Ω) is a 0603** carrying ~1.5 A. See 4e.
- **LT3083 VCONTROL is tied to the 5 V input**, so the output cannot reach the
  4.2 V the 1.6 A current loop asks for into 2 Ω. Current lands at ~1.3–1.6 A
  instead; fine for a burn, just not exactly regulated.
- The radio module (RF1) is on separate nets and does not affect any of this.

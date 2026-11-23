# Design notes

## Block diagram

```
Nano D11 (SER) --> U2 74HC595 (drive) --QH'--> U3 74HC595 (LEDs) --> 8 wire LEDs
                    |  QA..QH
                    v
              220R + 1N4148 per wire --> J1 near end ===cable=== J2 far end
                                                                  |  22k pulldown per wire
                                                                  v
Nano D12 (RX_DATA) <------------------------------------ U4 74HC165 (sense)
Nano D13 (SCK) clocks all three chips. D10 latches both 595s. D9 loads the 165.
```

## The scan

1. Put a single 1 in the drive register (wire 1), latch it. Wire 1 goes high, every other drive output is low.
2. Wait 100 us for the cable to settle.
3. Pulse /PL on the 74HC165 to capture all 8 far-end pins, then shift them in.
4. Store that byte as `rows[0]`. Repeat for wires 2 to 8.

For a good straight cable `rows[i]` has only bit `i` set. Anything else is a fault:

| `rows[i]` | Status |
|---|---|
| exactly bit `i` | OK |
| 0 | Open |
| one bit, not `i` | Crossed |
| two or more bits | Short (wire `i` touches another wire) |

If every wire lands on the mirror pin (wire 1 on pin n, wire 2 on pin n-1, ...) the cable is called reversed instead of failed.
All of this lives in `firmware/cable_tester/tester_logic.h`, which has no Arduino calls so it can be tested on a PC.

One full scan takes roughly 3 ms on a 16 MHz Nano: 8 passes of about 200 us for the two `shiftOut` calls, 100 us settle and a 60 us read.
The sketch scans every 200 ms, which is plenty for a human looking at LEDs.

## Why a diode on every drive line

Without the diodes, the undriven wires sit at a hard 0 V from the 595. If wire 4 is shorted to wire 5, driving wire 4 high pulls
against wire 5's low output through two series resistors, and the far end sits near 2.5 V. That is inside the 74HC165's
undefined input region, so the reading would be random.

With a 1N4148 in series, an undriven output can only sink current, not hold the wire low. The wire is pulled down by its 22k
at the far end only. A short then just puts more pulldowns in parallel.

## Voltage maths

Assumptions: 595 output high is about 4.9 V at this light load, 1N4148 drop 0.6 to 0.7 V, 22k pulldowns, 220R series resistor.

Single good wire:

```
I    = (4.9 - 0.6) / (220 + 22000)      = 0.19 mA
Vfar = 0.19 mA x 22k                     = 4.26 V
```

Worst case, all 8 wires shorted together (8 pulldowns in parallel = 2.75k):

```
I    = (4.9 - 0.65) / (220 + 2750)      = 1.43 mA
Vfar = 1.43 mA x 2.75k                   = 3.93 V
```

The 74HC165 needs about 0.7 x Vcc to read a 1, so 3.5 V at 5 V or 3.33 V if USB sags to 4.75 V. Both cases clear it.
The first draft used 10k pulldowns, which gave only 3.57 V in the all-shorted case. That was too close, so they went to 22k.

An undriven wire sits at 0 V through its 22k, well under the 1.5 V low threshold.

## Long cables

Cable capacitance is roughly 50 to 100 pF per metre per wire. After a wire was driven, it has to fall from 4.26 V to below 1.5 V
through its 22k pulldown before the next reading:

```
t = R x C x ln(4.26 / 1.5) = 1.04 x R x C
30 m at 100 pF/m: C = 3 nF, t = 1.04 x 22k x 3 nF = 69 us
```

The 100 us settle time covers about 40 m of typical cable. For longer cables raise `SETTLE_US` in the sketch.

## Current budget

| Load | Current |
|---|---|
| 8 wire LEDs, 1k each, yellow (about 2 V drop) | 8 x 2.9 mA = 23 mA from U3 (74HC595 limit is 70 mA total) |
| Pass and fail LEDs, 1k each, from Nano pins | about 3 mA each |
| Drive line, worst case all shorted | 1.43 mA |
| Three logic chips | under 1 mA |
| Nano itself | about 20 mA |

Total is well under 100 mA, so any USB port is fine.

## Shared clock

All three chips share one clock line (D13). When the 74HC165 is read, the 595s also see 8 clock edges and shift junk.
That is harmless: a 595 only changes its outputs on the latch pulse, and the next `writeRegisters` call shifts in a full fresh
16 bits before latching. The sketch reads the 165 manually instead of using `shiftIn`, because the 165 presents bit 7 straight
after the load pulse and `shiftIn` clocks before it reads, which would lose that bit.

## Safety and limits

- USB 5 V only. Never connect a cable that is plugged into a powered device or a mains appliance.
- The 220R resistors and diodes protect the 595 if a wire touches the far end of the same tester or a grounded shield, but they
  are not protection against external voltages.
- Continuity only. A joint of a few kilohms will still read as a 1. If you need a resistance check, use a meter.
- A wire shorted to the cable shield is not detected unless you wire the shield to one of the 8 positions.

## Ideas for v2

- Make the near and far ends separate boxes joined by the cable under test, for cables already installed in a wall.
- Measure each wire's resistance with an ADC and a known current.
- Add a small OLED that prints the full map ("1-1 2-2 3-6 ...") instead of only LEDs.
- Chain more 595 and 165 chips for 16 or 24 wires. The logic header would need `uint16_t` rows.

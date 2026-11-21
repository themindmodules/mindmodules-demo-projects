# Design notes

## How it works

Each colour channel is a low-side switch: the strip's common +12 V stays on, and an N-channel MOSFET
connects that colour's cathode wire to ground. The Nano's PWM output drives the MOSFET gate through a
100 ohm resistor; a 10 kohm resistor holds the gate low while the Nano boots, so the strip does not flash on at power-up.

| Part | Why |
|---|---|
| IRLZ44N | "Logic-level" MOSFET: fully on with 5 V on the gate. A normal IRF540-type needs higher gate voltage. |
| 100 ohm series gate resistor | Limits gate charging current and ringing |
| 10 kohm gate pulldown | Defined off state when the pin floats during boot or upload |
| D1 1N4007 | Feeds the Nano from 12 V through a diode, so the Nano is never back-fed from its VIN pin |
| C1 100 uF | Local reservoir near the strip connector for fast PWM edges |

## Current limits (do the arithmetic for your strip)

```
strip current = strip power (W) / 12 V
```

Example: a strip rated 36 W draws 3 A at full white. Per colour at full brightness that is roughly 1 A each.
The IRLZ44N is far above that. Limits you will actually hit:

- The **screw terminal and wiring** (use proper wire gauge for your total current).
- The **12 V supply** rating.
- **D1** only feeds the Nano, so it does not carry the strip current.

Add a **fuse** in the 12 V line for the strip (a few amps above the strip's normal current).

## Known limits of this version

- No reverse-polarity protection on the strip path (D1 protects only the Nano's VIN feed). A reversed supply would force current through the MOSFET body diodes and the strip. Mark the supply polarity clearly and use a fuse. A P-MOSFET ideal-diode stage is a good v2 addition.
- Default Arduino PWM is about 490 Hz. Phone cameras may show flicker. Faster PWM on D9/D10 (Timer1) and D11 (Timer2) is possible by changing the timer prescalers; do that only if you need it, and test.
- Brightness uses a simple squared curve, not a calibrated gamma table.
- Wire colour order of strips varies (some are R-G-B, some G-R-B). If colours look wrong, swap the wires or the pin definitions.
- No PCB in this package. The schematic is enough to lay one out; use the Nano footprint `Module:Arduino_Nano`.

## Ideas for v2

- Fuse holder and polyfuse on the input
- Reverse-polarity MOSFET
- Test points and a power LED
- PCB version with the same netlist
- IR remote or Bluetooth control instead of the button

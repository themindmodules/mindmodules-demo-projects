# Design notes

## Circuit

```
J1+ --> D1 (series, reverse protection) --> VIN --+--> U1 LM317 (IN)
                                                   |        |OUT --> VOUT --> J2+
                                                  C1       ADJ <--- R1 (240R) to VOUT
                                                   |        |
                                                  GND      RV1 (trimmer) to GND
 D2 across the regulator: anode VOUT, cathode VIN (protects U1 when the input collapses)
 C2 on the output smooths load steps
```

## Output voltage

```
Vout = 1.25 V x (1 + R2 / R1) + Iadj x R2
```

R1 = 240 ohm, R2 = trimmer 0-2 kohm, Iadj is about 50 uA (datasheet typical).

| Trimmer (R2) | Vout (approx.) |
|---|---|
| 0 ohm | 1.25 V |
| 500 ohm | 3.9 V |
| 1.0 kohm | 6.5 V |
| 1.5 kohm | 9.1 V |
| 2.0 kohm | 11.8 V |

Real values vary with resistor tolerance. Always set the output with a multimeter before connecting a load.

## Why each part is there

| Part | Reason |
|---|---|
| D1 1N4007 in series | Blocks reversed input polarity. Costs about 0.7 V and is the **1 A current limit** of this board. |
| D2 1N4007 across U1 | If the input is shorted, C2 could discharge backwards through the LM317 and damage it. D2 gives that current another path. |
| C1 470 uF 35 V | Input reservoir. Rated above the 24 V maximum input. |
| C2 100 uF 25 V | Output decoupling. Rated well above 11.8 V. |
| R1 240 ohm | Standard LM317 reference resistor. It also provides about 5 mA minimum load at 1.25 V. |
| RV1 wiper tied to one end | If the wiper loses contact while turning, the output does not jump to maximum. |

**Minimum load:** the LM317 wants at least a few mA of load to regulate. R1 provides 5 mA at the lowest
setting, which meets the typical figure, but the datasheet allows up to 10 mA. For guaranteed regulation with no load,
use 120 ohm for R1 (then Vout max changes to about 22 V with 2 k, so use a 1 k trimmer instead).

## Heat is the real limit

A linear regulator burns the difference as heat:

```
P = (Vin - 0.7 V - Vout) x Iload
```

| Vin | Vout | Load | Heat in U1 | What you need |
|---|---|---|---|---|
| 16 V | 12 V | 0.3 A | about 1.0 W | Probably fine without heatsink |
| 18 V | 5 V | 0.5 A | about 6.2 W | Heatsink required |
| 24 V | 5 V | 1.0 A | about 18 W | **Do not.** Too much for this board; use a buck converter instead. |

Rule of thumb: above about 1.5 W, fit a heatsink. Check the LM317 datasheet thermal figures for your exact
package and heatsink before running near the limit. The LM317 has thermal shutdown, but running it there for long shortens its life.

## Layout notes

- Back copper is a ground pour; power tracks are 1.0 mm on the front.
- Input, output and adjust sections are kept in separate areas so the adjust resistors stay away from the power path.
- Mounting holes at the bottom corners for M3 screws or standoffs.
- U1 is mounted vertically. For a heatsink, bend the leads to lay it flat, or use a clip-on vertical heatsink.
- Check the terminal block wire entry direction against the 3D/silkscreen when you receive the board and
  orient your wires accordingly.

## Known limits of this version

- No input fuse and no output current limit (add a fuse at J1 for safety).
- No indicator LED and no test points (easy v1.1 additions).
- Not validated on a bench. Numbers above are calculations from datasheet typicals, not measurements.

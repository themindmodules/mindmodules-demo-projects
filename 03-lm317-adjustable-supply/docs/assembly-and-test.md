# Assembly and test plan

## Parts (see `bom/bom.csv`)

C1 470 uF 35 V, C2 100 uF 25 V, D1 and D2 1N4007, R1 240 ohm, RV1 2 kohm 3296W trimmer,
U1 LM317T (TO-220), J1 and J2 2-pin 5.08 mm screw terminals, optional heatsink.

## Build order (low to high parts)

1. D1 and D2 (check the band: it marks the cathode, which matches the **K** on the silkscreen).
2. R1.
3. RV1 trimmer.
4. U1 (flat side orientation follows the silkscreen outline).
5. C1 and C2: **electrolytic caps are polarised.** The long lead goes to the pad marked **+**; the stripe on the can marks the negative side.
6. J1 and J2.

Inspect the solder joints under good light before powering anything.

## Test plan

Use a bench supply with current limit set to 100 mA for steps 1-4 if you have one. If you do not, use a 9-12 V
wall adapter through a 100 ohm resistor first.

| # | Step | Expected |
|---|---|---|
| 1 | Resistance between J1+ and J1- with the board unpowered | Not a short (should read open or charge up as the cap charges) |
| 2 | Apply 15 V at J1 (correct polarity), no load. Measure J2. | A voltage between about 1.2 V and 12 V; it changes as you turn RV1 |
| 3 | Turn RV1 fully one way, then the other | Output sweeps over roughly 1.25 V to about 12 V |
| 4 | Set 5.0 V on the output with a multimeter | Stable reading |
| 5 | Reverse the input polarity briefly with the current limit still set | Nothing conducts (D1 blocks); no heat, no smoke |
| 6 | Connect a 100 ohm resistor load at 5 V (about 50 mA) | Output stays at 5 V within a few tens of mV |
| 7 | Load 5 V at about 0.5 A (10 ohm, 5 W resistor), input 12 V | Output holds; U1 warm. Measure temperature after 5 minutes. |
| 8 | Remove load, input collapse test: switch input off | Output decays smoothly; U1 stays cool |

Stop and recheck wiring if any part gets hot in steps 1-6.

## Record your results

Copy this table and fill in measured values. Real measurements are more valuable than the calculated ones in `design-notes.md`.

| Trimmer position | Measured Vout | Load | Measured Vout under load |
|---|---|---|---|
| min | | | |
| mid | | | |
| max | | | |

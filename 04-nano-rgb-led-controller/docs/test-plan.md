# Test plan (in this order)

Do not connect the strip until step 6 passes.

| # | Step | Expected |
|---|---|---|
| 1 | Visual check: no solder bridges, MOSFET legs correct (G, D, S), diode band toward the Nano side | Looks right |
| 2 | Multimeter, power off: GND rail to 12 V rail | Not a short (some resistance or open) |
| 3 | Power the Nano over USB only (12 V disconnected), upload the sketch | Upload succeeds |
| 4 | Multimeter on each MOSFET gate (relative to GND) while turning the pot and pressing the button | Gate voltage changes with mode and brightness (0 to about 5 V average on PWM) |
| 5 | Apply 12 V (strip still disconnected). Measure the Nano 5V pin and VIN pin | 5V pin about 5 V; VIN about 11.3 V |
| 6 | Connect a **test load first**: a single 12 V LED or a 1 kohm resistor from J2 pin 1 (+12 V) to J2 pin 2 (red) | Resistor/LED brightness follows the red channel |
| 7 | Repeat step 6 for pins 3 and 4 | Green and blue channels behave the same |
| 8 | Connect the real strip, pot fully down first | Strip stays off or dim, then follows the pot |
| 9 | Press the button through all modes | Warm white, rainbow, breathing blue, off |
| 10 | Run at full white for 10 minutes | MOSFETs only slightly warm; wires and terminals cool |

If a MOSFET gets hot at full brightness, recheck the gate drive in step 4 (it should reach about 5 V in full-on mode).

## Run the logic tests on a PC (optional, needs g++)

```sh
cd firmware/test
g++ -std=c++11 logic_check.cpp -I../rgb_lamp -o logic_check
./logic_check            # prints ALL PASS
```

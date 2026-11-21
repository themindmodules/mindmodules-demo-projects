# Perfboard build guide

Board: about 5 x 7 cm perfboard. Build the logic and MOSFET stage first, test it, then connect the strip.

## Parts (summary, full list in `bom/bom.csv`)

Arduino Nano (with headers), 3x IRLZ44N (TO-220), 3x 100 ohm, 3x 10 kohm, 1N4007, 100 uF 25 V
electrolytic, 10 kohm pot, push button, 2-pin and 4-pin 5.08 mm screw terminals, 12 V DC supply sized for your strip.

## Connections

Use short wires and a thick common ground. Every row is one connection.

### Power

| From | To |
|---|---|
| 12 V supply + (J1 pin 1) | J2 pin 1 (strip common +12 V), C1 +, D1 anode |
| D1 cathode | Nano **VIN** |
| 12 V supply - (J1 pin 2) | Common GND rail |
| C1 - | GND rail |
| Nano GND (either GND pin) | GND rail |

### Signals (one per colour)

| Channel | Nano pin | Series R (100 ohm) to | Gate pulldown (10 kohm) | MOSFET (G, D, S) | Strip terminal |
|---|---|---|---|---|---|
| Red | D9 | gate of Q1 | gate to GND | G=gate, D=J2 pin 2, S=GND | J2 pin 2 |
| Green | D10 | gate of Q2 | gate to GND | G=gate, D=J2 pin 3, S=GND | J2 pin 3 |
| Blue | D11 | gate of Q3 | gate to GND | G=gate, D=J2 pin 4, S=GND | J2 pin 4 |

IRLZ44N pin order (facing the printed side, legs down): **Gate, Drain, Source** from left to right.

### User controls

| Part | Connection |
|---|---|
| Pot outer pins | Nano 5V and GND |
| Pot wiper (middle pin) | Nano **A0** |
| Button | One side to **D2**, other side to GND (no resistor needed; the code uses the internal pull-up) |

## Build order

1. Solder the Nano's socket/headers, GND rail and 12 V rail first.
2. Add the three MOSFETs, gate resistors and pulldowns. Check with a multimeter that no gate is shorted to a drain.
3. Add D1 and C1.
4. Wire pot and button.
5. **Do not connect the strip yet.** Go to `test-plan.md`.

## Mistakes to avoid

- Swapping drain and source on a MOSFET (strip will glow dimly or not switch).
- Forgetting the common ground between 12 V supply and Nano.
- Using a common-cathode (shared ground) RGB strip: this circuit only drives **common-anode (+12 V common)** strips.
- Powering the Nano from USB and 12 V at the same time through VIN without D1 (D1 prevents back-feeding).

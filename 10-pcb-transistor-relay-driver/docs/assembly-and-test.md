# Assembly and test plan

This plan has not been run on a built board yet. The expected readings come from the hand
calculations in `design-notes.md`, so allow for part tolerances.

## Tools

- 5 V bench supply with a current limit (or a USB power board and a multimeter in series)
- Multimeter
- A jumper wire, and a 3.3 V source for the logic test (a 3.3 V microcontroller pin works)
- Optional: an oscilloscope to look at the collector during turn off

## Build order

Solder low parts first, tall parts last.

1. Resistors R1 to R6. R1 and R3 are 470 ohm, R2 and R4 are 10k, R5 and R6 are 1k.
2. Diodes D1 and D2 (1N4148). The black band is the cathode and goes to the square pad, the side marked K.
3. Transistors Q1 and Q2 (BC337). Flat face as shown on the silkscreen. If you use any other transistor, read the pinout warning in `design-notes.md` first.
4. LEDs D3 and D4. The short leg (cathode) goes to the square pad.
5. C2 (100 nF), then C1 (100 uF). C1 is polarised: the long leg goes to the pad marked +.
6. Header J4, terminal blocks J1, J2 and J3.
7. Relays K1 and K2 last. They only fit one way.
8. Check for solder bridges, especially between the three closely spaced transistor pins.

## Test 1: power off checks

With nothing connected, measure resistance:

| Between | Expected |
|---|---|
| J3 +5V and GND | not a short. It reads low at first and climbs as C1 charges from the meter |
| J4 IN1 and GND | about 10.5k (470 ohm plus 10k). It can read lower if your meter turns on the base junction |
| Relay 1 COM and NC (J1) | continuity, near 0 ohm |
| Relay 1 COM and NO (J1) | open |

Repeat the last three for channel 2. If COM to NC is open and COM to NO is closed, your relay has
its contacts the other way round from the footprint. Swap the wires in your project, not the part.

## Test 2: idle current

1. Set the supply to 5.0 V with a 300 mA current limit.
2. Connect it to J3. Leave IN1 and IN2 unconnected.
3. Supply current should be close to 0 mA. Both LEDs off. No relay click.

If a relay clicks with the inputs floating, check R2 and R4 (the pull-downs).

## Test 3: switch with 5 V logic

1. Connect IN1 on J4 to 5V on J4 with a jumper.
2. Expect a click, D3 lights, and the supply current rises by about 74 mA.
3. Measure Q1 collector (pin 1) to ground. Expect 0.2 V or less.
4. Measure Q1 base to ground. Expect about 0.7 to 0.8 V.
5. J1: COM to NO now shows continuity, COM to NC is open.
6. Remove the jumper. The relay releases and D3 goes out.

Repeat for IN2.

## Test 4: switch with 3.3 V logic

1. Drive IN1 from a 3.3 V source (a microcontroller pin set high works).
2. The relay should pull in as in Test 3. Q1 collector should still read 0.2 V or less.
3. If the collector reads higher than about 0.5 V, the transistor is not saturated. Check R1 and the transistor type.

## Test 5: flyback (optional, needs a scope)

1. Probe Q1 collector, ground clip on GND.
2. Toggle IN1 on and off.
3. At turn off the collector should rise to about 5.8 V and settle at 5 V. A tall spike (tens of volts) means D1 is missing, open or fitted the wrong way.

## Test 6: contact load

Use a low voltage load only, for example a 12 V lamp or a 12 V fan, under 3 A.
Put it in series with COM and NO on J1, switch it with IN1 and check it turns on and off cleanly.
Never connect mains to this board.

## Example sketch

A minimal Arduino sketch that clicks relay 1 and relay 2 in turn. Wire IN1 to pin 7, IN2 to pin 8,
and the J4 GND to the Arduino GND.

```cpp
const int RELAY1 = 7;
const int RELAY2 = 8;

void setup() {
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  digitalWrite(RELAY1, LOW);   // active high, LOW = off
  digitalWrite(RELAY2, LOW);
}

void loop() {
  digitalWrite(RELAY1, HIGH);
  delay(1000);
  digitalWrite(RELAY1, LOW);
  digitalWrite(RELAY2, HIGH);
  delay(1000);
  digitalWrite(RELAY2, LOW);
}
```

The grounds must be shared. Without a common ground the base current has no return path and the
relay will not switch.

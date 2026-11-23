# Perfboard build

A 9 x 15 cm perfboard (or two 7 x 9 cm boards) holds everything. Use 16-pin DIP sockets for the three chips
and female headers for the Nano so you can pull it out later.

## Suggested layout

```
 +---------------------------------------------------------------+
 |  [Nano on female headers]      U2 595    R1..R8  D1..D8   J1 |
 |   USB to the left edge          (drive)   220R   1N4148  near |
 |                                                               |
 |  SW1                            U4 165    R9..R16 (22k)   J2 |
 |                                 (sense)                   far |
 |                                                               |
 |  C1 C2 C3 next to each chip     U3 595    R17..R24  D9..D16   |
 |                                 (LEDs)    1k        wire LEDs |
 |                                           R25 D17 green       |
 |                                           R26 D18 red         |
 +---------------------------------------------------------------+
```

Run a +5V rail along the top edge and a GND rail along the bottom. Keep each 100nF cap within a few mm of its chip's pin 16.
Put the 8 wire LEDs in a straight row in order 1 to 8, so the LED position matches the wire number.

## Build order

1. Sockets, Nano headers, power rails. Check with a meter that +5V and GND are not shorted.
2. Decoupling caps C1 (U2), C2 (U4), C3 (U3).
3. Control wiring between the Nano and the three sockets (table below).
4. Drive path: R1..R8 and D1..D8 to J1. Watch the diode direction: band (cathode) towards J1.
5. Sense path: J2 to U4 inputs, with R9..R16 from each J2 pin to GND.
6. LEDs: R17..R24 and D9..D16, then R25/D17 and R26/D18. LED long leg (anode) towards the resistor.
7. Button SW1 between D2 and GND.
8. Do the checks in `test-plan.md` before you insert the chips.

## Wiring table: power and control

| From | To | Net |
|---|---|---|
| Nano +5V | U2 pin 16, U2 pin 10, U3 pin 16, U3 pin 10, U4 pin 16, C1, C2, C3 | +5V |
| Nano GND | U2 pin 8, U2 pin 13, U3 pin 8, U3 pin 13, U4 pin 8, U4 pin 10, U4 pin 15, C1, C2, C3 | GND |
| Nano D11 | U2 pin 14 (SER) | SER |
| U2 pin 9 (QH') | U3 pin 14 (SER) | SER2 |
| Nano D13 | U2 pin 11, U3 pin 11, U4 pin 2 | SCK |
| Nano D10 | U2 pin 12, U3 pin 12 | LATCH |
| Nano D9 | U4 pin 1 (/PL) | RX_LOAD |
| Nano D12 | U4 pin 9 (Q7) | RX_DATA |
| Nano D2 | SW1 one side (other side to GND) | BTN |
| Nano D3 | R25 then D17 green anode, cathode to GND | PASS |
| Nano D4 | R26 then D18 red anode, cathode to GND | FAIL |

U2 pin 13 (/OE) and U3 pin 13 go to GND so the outputs are always enabled. Pin 10 (/SRCLR) goes to +5V.
On U4, pin 15 (/CE) and pin 10 (DS) go to GND. U4 pin 7 and U3 pin 9 are not connected.

## Wiring table: per wire

The 595 output pins are not in order around the chip: QA is pin 15, QB to QH are pins 1 to 7.
The 165 inputs are split too: D0 to D3 are pins 11 to 14, D4 to D7 are pins 3 to 6.

| Wire | U2 out | 220R | 1N4148 (band to J1) | J1 pin | J2 pin | 22k to GND | U4 in | U3 out | 1k | LED |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 | pin 15 (QA) | R1 | D1 | 1 | 1 | R9 | pin 11 (D0) | pin 15 (QA) | R17 | D9 |
| 2 | pin 1 (QB) | R2 | D2 | 2 | 2 | R10 | pin 12 (D1) | pin 1 (QB) | R18 | D10 |
| 3 | pin 2 (QC) | R3 | D3 | 3 | 3 | R11 | pin 13 (D2) | pin 2 (QC) | R19 | D11 |
| 4 | pin 3 (QD) | R4 | D4 | 4 | 4 | R12 | pin 14 (D3) | pin 3 (QD) | R20 | D12 |
| 5 | pin 4 (QE) | R5 | D5 | 5 | 5 | R13 | pin 3 (D4) | pin 4 (QE) | R21 | D13 |
| 6 | pin 5 (QF) | R6 | D6 | 6 | 6 | R14 | pin 4 (D5) | pin 5 (QF) | R22 | D14 |
| 7 | pin 6 (QG) | R7 | D7 | 7 | 7 | R15 | pin 5 (D6) | pin 6 (QG) | R23 | D15 |
| 8 | pin 7 (QH) | R8 | D8 | 8 | 8 | R16 | pin 6 (D7) | pin 7 (QH) | R24 | D16 |

Each LED cathode goes to GND.

## Adapters for real cables

J1 and J2 are plain 8-pin headers. For a specific cable type, solder a matching breakout to each:

| Cable | Breakout | Notes |
|---|---|---|
| Ethernet (RJ45) | RJ45 jack breakout, pins 1 to 8 in order | Straight patch cable shows green, rollover shows blinking green. A crossover cable shows red with wires 1, 2, 3, 6 blinking, which is expected |
| Ribbon or Dupont | 2.54 mm header | Set the wire count with the button |
| JST or servo leads | Matching JST socket | Set the wire count with the button |

Label which board end is "near" and which is "far" so the wire numbers on the LEDs match the cable.

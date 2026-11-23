# Assembly and test plan

Topic suggested by Sujan Ghosh on the @themindmodules Instagram channel.

This plan has not been run on a built board. Expected readings come from the hand calculations in
`design-notes.md`, so allow for the part tolerances.

## Tools

- Variable bench supply, 0 to 20 V, with current limit
- Multimeter
- Four resistors of about 100 ohm for dummy cells (see the balancer test)
- A power resistor or lamp for a load, 2 ohm to 10 ohm, 25 W or more

## Build order

Solder low parts first, tall parts last.

1. Resistors R1 to R20 (R21 is the large 2 W shunt, fit it last of the resistors).
2. D1 zener. The stripe is the cathode and goes to the square pad.
3. IC socket for U5 (use a DIP-8 socket, then fit the LM393).
4. U1 to U4 and U6 (TL431). Check the pin order against your datasheet first, see `design-notes.md`.
5. Capacitors C1 to C3.
6. Q1 (IRLZ44N), then the terminal blocks J1 and J2, then the cell tap header J3.
7. Clean the board with isopropyl alcohol and check for solder bridges under a magnifier.

## Test 1: power off checks

With nothing connected, measure resistance:

| Between | Expected |
|---|---|
| Pack B+ and B- on J1 | above 10 kilohm, and not close to zero (the dividers and reference are in parallel) |
| Q1 drain (pin 2) and J2 P- | continuity, no resistance |

## Test 2: reference

1. Set the bench supply to 12.8 V with a 20 mA current limit.
2. Connect to J1 only (B+ and B-). Leave J3 and J2 unconnected.
3. Measure U6 pin 1 to ground. Expect 2.495 V, within about 1 percent.
4. Measure U5 pin 8 to ground, expect 12.8 V. Measure U5 pin 3 (UVDIV), expect about 2.89 V.
5. Measure R19 top (OCTH), expect about 0.2 V.
6. Measure Q1 gate to ground. Expect close to 12.8 V (the gate is pulled up and the MOSFET is on).

## Test 3: under-voltage cutoff

1. Put a 100 ohm resistor from J2 P+ to P- as a light load.
2. Slowly lower the supply from 12.8 V. Watch the gate voltage.
3. The gate should fall to near 0 V at about 11.04 V.
4. Raise the supply. The gate should rise again at about 11.21 V.
5. Record both values. A few hundred millivolts of difference from the calculation is normal if
   the 1 percent resistors are not exact.

## Test 4: over-current cutoff

1. Set the supply to 12.8 V with a current limit of 12 A.
2. Connect a load that draws about 5 A, for example 2.5 ohm 50 W.
3. Confirm the gate stays high and the MOSFET stays cool.
4. Reduce the load resistance in steps until the current reaches about 10 A. The gate should
   switch off and the load voltage drop to zero, then the circuit will try again (hiccup).
5. Do not run a dead short. The supply current limit should stay below 12 A.

## Test 5: balancer with fake cells

Real cells are not needed. Use four bench supply channels, or one supply with a resistor chain.

1. For each cell input, set the voltage between the taps to 3.3 V, then slowly raise it.
2. At about 3.60 V the TL431 should start to conduct. Measure the voltage across R9 (the 18 ohm
   resistor). At 3.65 V expect about 1.1 to 1.5 V across it, which is 60 to 85 mA.
3. Check all four channels. They should start within about 30 mV of each other.

If you only have one supply, test each stage by feeding the cell tap pins one cell at a time
with the others held at 3.3 V by a second supply.

## Test 6: first real cells

Only after tests 1 to 5 pass, and after reading `limits-and-safety.md`.

1. Measure each cell. Confirm they are within 0.1 V of each other, between 3.0 V and 3.4 V.
2. Connect the cell taps in order: B-, C1, C2, C3, then B+.
3. Connect J1 to the pack through a fuse of 10 A or less.
4. Check the gate voltage and the voltage at J2. With the load off, J2 P+ should equal the pack
   voltage.
5. Charge with a LiFePO4 charger set to 14.4 V and watch the cell voltages at the end of charge
   for the first few cycles.

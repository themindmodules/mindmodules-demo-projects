# Design notes

Topic suggested by Sujan Ghosh on the @themindmodules Instagram channel.

All values below are hand calculations from datasheet typical numbers. Nothing here was simulated
or measured. Use 1 percent resistors where the BOM says so, because the thresholds depend on them.

## Block diagram

```
 cell taps (J3) --> 4 x [ TL431 shunt balancer ]          (per cell, 3.60 V)

 pack + (J1) -----+--------------------------+----------------> load + (J2)
                  |                          |
              [ 6.8k + TL431 ]           [ 34.8k / 10k ]
                  | VREF 2.495 V             | UVDIV
                  |                          |
        +---------+-------- LM393 -----------+
        |        OC comp          UV comp
   [ 11.5k / 1k ]  |                |
        | OCTH     | OCSENSE        | outputs share one net: GATE
        |          |                |
 pack - (J1) --[ 0.02 ohm ]-- SRC --+-- IRLZ44N --> load - (J2)
```

The MOSFET sits in the negative line. Pack minus is the board ground. The load return goes
through the MOSFET and the shunt.

## Per cell balancer (four identical copies)

Each TL431 is wired as an adjustable shunt across one cell. R1 (4.42k) and R5 (10k) divide the
cell voltage into the reference pin.

    Vstart = 2.495 V x (1 + 4.42k / 10k) = 3.598 V

When a cell goes above 3.598 V the TL431 turns on and pulls current from the cell through the
18 ohm bleed resistor (R9). Bleed current is the cell voltage minus the TL431 cathode voltage,
divided by 18 ohm:

| Cell voltage | TL431 in regulation (cathode 2.5 V) | TL431 saturated (cathode about 2.1 V) |
|---|---|---|
| 3.60 V | 61 mA | 83 mA |
| 3.65 V | 64 mA | 86 mA |
| 3.70 V | 67 mA | 89 mA |

The TL431 absolute maximum is 100 mA, and 18 ohm keeps the worst case under that. Dissipation is
about 0.16 W in the TL431 and about 0.07 W in the resistor, both inside a TO-92 and a quarter watt
part. Balancing only works while the charger current is small, so it acts at the end of charge
when the charger tapers. It will not rescue a pack that is badly out of balance, see
`limits-and-safety.md`.

Each divider draws 3.3 V / 14.42k = 0.23 mA from its own cell. All four draw the same, so the
dividers do not unbalance the pack.

## Reference

U6 (TL431) with its reference pin tied to the cathode gives a fixed 2.495 V. R13 (6.8k) feeds it
from the pack. The reference current is (pack - 2.495) / 6.8k:

| Pack voltage | Reference current |
|---|---|
| 11.0 V | 1.25 mA |
| 12.8 V | 1.52 mA |
| 14.6 V | 1.78 mA |

The TL431 needs at least 1 mA to regulate, so the lowest working pack voltage is about 9.3 V.
C1 (100 nF) keeps the reference quiet.

## Under-voltage cutoff

R14 (34.8k) and R15 (10k) divide the pack voltage into comparator input IN+ of U5A. The
reference goes to IN-. The output is open collector and pulls the gate net down when IN+ falls
below IN-.

    Vdiv = Vpack x 10k / (34.8k + 10k) = 0.2232 x Vpack

R16 (2.2M) feeds the gate net back into the divider node, which adds hysteresis. Solving the
divider with the feedback in both states:

| Event | Pack voltage | Per cell |
|---|---|---|
| Load switches off (falling) | 11.04 V | 2.76 V |
| Load switches on again (rising) | 11.21 V | 2.80 V |

LiFePO4 voltage rises very little when the load is removed, so the board may chatter at the
threshold under a heavy load. The hysteresis limits this but does not remove it.

## Over-current cutoff

R21 is a 0.02 ohm 2 W shunt between pack minus and the MOSFET source. R18 (11.5k) and R19 (1k)
divide the reference down to the trip level:

    Vth = 2.495 V x 1k / (11.5k + 1k) = 0.1996 V
    Itrip = 0.1996 V / 0.02 ohm = 9.98 A

R20 (1k) and C3 (100 nF) filter the sense voltage with a time constant of 100 microseconds, so
short spikes do not trip it. The LM393 input offset is a few millivolts, which moves the trip
point by roughly 0.25 to 0.5 A. After a trip the load is off, the current drops to zero, the
comparator releases and the MOSFET turns on again, so a hard short will repeat on and off
(hiccup). Put a fuse on the pack positive lead. The fuse is the real short circuit protection.

## Gate drive

R17 (100k) pulls the gate up to the pack positive. Both comparator outputs are open collector, so
either one can pull the gate to ground and switch the MOSFET off. D1 (15 V zener) clamps the gate
to source voltage below the 16 V limit of the IRLZ44N. The IRLZ44N is a logic-level part, so it
is fully on with 12 V of gate drive. At 5 A the on-resistance of about 0.025 ohm loses roughly
0.6 W, so the TO-220 can run without a heatsink at 5 A in open air, but add one for anything
sustained above that.

Charging current flows the other way, through the body diode of the MOSFET (about 0.7 V drop),
and through the shunt. At 5 A charging that diode dissipates about 3.5 W, so for charging
current above 2 A fit a heatsink or leave the MOSFET on. When the pack is above the cutoff
voltage the MOSFET is already on during charging, so the diode only conducts after an
under-voltage cutoff.

## Copper and shunt limits

The 2 mm wide power tracks on 1 oz copper carry roughly 5 A for a small temperature rise, which
sets the continuous rating of the board. The shunt dissipates 0.5 W at 5 A and 2 W at 10 A, and
its 2 W rating is only for short trips.

## Idle drain

| Source | Current |
|---|---|
| Reference (U6 and R13) | 1.5 mA |
| LM393 | about 0.8 mA |
| Pack divider R14/R15 | 0.29 mA |
| Four cell dividers | 0.23 mA each |

About 2.6 mA from the whole pack, plus 0.23 mA extra on each cell. On a 10 Ah pack that is
roughly 0.6 percent per day. The board keeps drawing after an under-voltage cutoff, so do not
leave a cut off pack connected for days.

## TL431 pin order warning

The KiCad symbol and footprint use pin 1 = reference, 2 = anode, 3 = cathode. Makers differ.
Some TO-92 TL431 parts list the pins in another order. Check the datasheet of the exact part you
bought and rotate it if needed. A wrong orientation will not work and may be damaged.

## Layout notes

- Ground is a pour on the bottom layer, tied to every ground pad, so ground has no routed tracks.
- Signal tracks are 0.4 mm. The 2 mm power tracks (pack positive, shunt to MOSFET, MOSFET to load
  return) are placed by hand.
- Cell tap pins are on a 2.54 mm header at the top. Pin order from left to right is B+, C3, C2, C1,
  B-. Connect each tap wire to the cell positive terminal for that step, with C1 closest to B-.

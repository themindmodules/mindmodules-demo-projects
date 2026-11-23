# Limits and safety

Read this before you connect a real battery. A LiFePO4 pack can deliver a very large current into
a short circuit, and a bad connection order on the cell taps can destroy parts.

## What this board protects against

- Balancing: each cell is bled once it passes 3.60 V, if the charger is still pushing current.
- Pack under-voltage: the load is switched off at about 11.04 V.
- Over-current: the load is switched off at about 10 A.

## What it does NOT protect against

| Missing feature | What to do about it |
|---|---|
| Over-charge disconnect | Use a CC/CV LiFePO4 charger set to 14.4 V (3.6 V per cell). Do not use a charger that has no voltage limit. |
| Per cell under-voltage | A weak cell can drop below 2.5 V while the pack total is still above 11 V. Check the cells with a meter, and watch for a cell that is clearly lower than the rest. |
| Short circuit speed | The comparator and gate path react in microseconds to milliseconds, which is too slow for a dead short on a large pack. Fit a fuse or breaker on pack positive. |
| Over-temperature | There is no temperature sensor. Do not charge LiFePO4 below 0 C. |
| Reverse polarity on the pack | There is no reverse protection. Check polarity twice before connecting. |
| Large imbalance | The balance current is only about 60 to 85 mA. Bring cells to within about 0.1 V of each other (parallel them for a few hours, or charge them individually) before you fit the board. |

## Rules for a first build

1. Test the finished board on a bench supply and resistors first, using `assembly-and-test.md`.
   Only then move to real cells.
2. Connect the cell taps in this order: B- first, then C1, C2, C3, then B+. Disconnect in the
   reverse order. A wrong order can put the full pack voltage across a part for a moment.
3. Put a fuse of 10 A or less on the pack positive lead.
4. Do not leave the pack connected to the board in storage. The board draws about 2.6 mA.
5. Keep the pack voltage between 10 V and 14.6 V. The board is not designed for lead acid or
   Li-ion packs, whose voltages are different.
6. Do not use it above 5 A continuous.
7. Work on a non-flammable surface and keep a fire blanket or sand nearby when you first charge
   real cells.

## Not for

Vehicles, medical devices, anything unattended, or any pack above 14.6 V. For those you need a
certified BMS.

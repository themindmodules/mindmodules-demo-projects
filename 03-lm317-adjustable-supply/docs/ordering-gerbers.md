# Ordering the PCB

The files in `hardware/gerbers/` are standard RS-274X Gerbers plus an Excellon drill file, exported from KiCad.

## Steps

1. Zip the contents of `hardware/gerbers/` (the `.gtl`, `.gbl`, `.gts`, `.gbs`, `.gto`, `.gbo`, `.gm1`, `.drl` files). The `.gbrjob` file is not needed for fabrication.
2. Upload the zip to your PCB manufacturer's website. They will show a preview. Check that the outline, holes and the green board colour look right.
3. Choose these options:

| Option | Value |
|---|---|
| Layers | 2 |
| Size | 66 x 38 mm |
| Thickness | 1.6 mm |
| Copper | 1 oz |
| Surface finish | HASL (lead-free if available) is fine for through-hole |
| Quantity | 5 (usual minimum) |

4. Through-hole assembly is done by hand; no pick-and-place needed.

## Before you pay

- Open the preview and zoom on the terminal blocks and U1 pads; compare with `hardware/docs/lm317_supply-pcb-top.png`.
- Check that the manufacturer's own DRC passes with no warnings about hole sizes or spacing.
- Different fabs have slightly different rules; if one raises a flag, tell them the design was checked in KiCad with 0 DRC violations and ask which rule triggered.

## What the layers are

| File | Layer |
|---|---|
| `*-F_Cu.gtl` | Front copper (power tracks) |
| `*-B_Cu.gbl` | Back copper (ground pour) |
| `*-F_Mask.gts`, `*-B_Mask.gbs` | Solder mask |
| `*-F_Silkscreen.gto`, `*-B_Silkscreen.gbo` | Silkscreen (labels) |
| `*-Edge_Cuts.gm1` | Board outline |
| `*.drl` | Drill holes |

# Ordering the PCB

The files in `hardware/gerbers/` are standard RS-274X Gerbers plus an Excellon drill file, exported from KiCad.

## Steps

1. Zip the `.gtl`, `.gbl`, `.gts`, `.gbs`, `.gto`, `.gbo`, `.gm1` and `.drl` files from `hardware/gerbers/`.
   The `.gbrjob` and `positions.csv` files are not needed for a bare board order.
2. Upload the zip to your PCB maker. Check their preview: outline, holes and silkscreen labels.
3. Choose these options:

| Option | Value |
|---|---|
| Layers | 2 |
| Size | 64 x 76 mm |
| Thickness | 1.6 mm |
| Copper | 1 oz |
| Surface finish | HASL (lead-free if offered) is fine for through-hole |
| Quantity | 5 (usual minimum) |

4. All parts are through-hole and soldered by hand, so no assembly service is needed.

## Before you pay

- Compare the preview with `hardware/docs/relay_driver-pcb-top.png`.
- Zoom in on the relay footprints and check all five holes are there for each relay.
- If the maker's own check flags something, the design was checked in KiCad with 0 DRC violations.
  Ask them which of their rules was hit; the hole sizes and spacing are normal for through-hole parts.

## What the layers are

| File | Layer |
|---|---|
| `*-F_Cu.gtl` | Top copper (relay contact tracks and most signals) |
| `*-B_Cu.gbl` | Bottom copper (ground pour under the driver area, a few signal tracks) |
| `*-F_Mask.gts`, `*-B_Mask.gbs` | Solder mask |
| `*-F_Silkscreen.gto`, `*-B_Silkscreen.gbo` | Silkscreen labels |
| `*-Edge_Cuts.gm1` | Board outline |
| `*.drl` | Drill holes |

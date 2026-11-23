#!/bin/sh
# usage: export.sh <repo_root> <project_name>
# Exports gerbers, drill, pick&place, BOM, PDFs, SVG and DRC/ERC reports with kicad-cli.
CLI="/d/KICAD INSTALLATION/bin/kicad-cli.exe"
R="$1"; N="$2"; K="$R/hardware/kicad"
mkdir -p "$R/hardware/gerbers" "$R/hardware/reports" "$R/hardware/docs" "$R/bom"
"$CLI" pcb export gerbers --layers "F.Cu,B.Cu,F.Silkscreen,B.Silkscreen,F.Mask,B.Mask,Edge.Cuts" --use-drill-file-origin -o "$R/hardware/gerbers/" "$K/$N.kicad_pcb"
"$CLI" pcb export drill --format excellon --drill-origin absolute -o "$R/hardware/gerbers/" "$K/$N.kicad_pcb"
"$CLI" pcb export pos --format csv --units mm -o "$R/hardware/gerbers/$N-positions.csv" "$K/$N.kicad_pcb"
"$CLI" pcb drc --severity-all --format report -o "$R/hardware/reports/drc.rpt" "$K/$N.kicad_pcb"
"$CLI" sch erc --severity-all -o "$R/hardware/reports/erc.rpt" "$K/$N.kicad_sch"
"$CLI" sch export pdf -o "$R/hardware/docs/$N-schematic.pdf" "$K/$N.kicad_sch"
"$CLI" sch export bom -o "$R/bom/bom.csv" "$K/$N.kicad_sch"
"$CLI" sch export netlist --format kicadxml -o "$R/hardware/reports/netlist.xml" "$K/$N.kicad_sch"
"$CLI" pcb export pdf --layers "F.Cu,B.Cu,F.Silkscreen,Edge.Cuts" -o "$R/hardware/docs/$N-pcb.pdf" "$K/$N.kicad_pcb"
"$CLI" pcb export svg --layers "F.Cu,B.Cu,F.Silkscreen,Edge.Cuts" --page-size-mode 2 --exclude-drawing-sheet -o "$R/hardware/docs/$N-pcb.svg" "$K/$N.kicad_pcb"
echo EXPORT_DONE

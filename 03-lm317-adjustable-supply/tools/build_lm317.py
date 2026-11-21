"""Generate the LM317 adjustable supply: KiCad schematic + routed PCB.
Run with KiCad's bundled Python:  <KiCad>/bin/python.exe build_lm317.py <repo_root>
"""
import os
import sys

import pcbnew

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import kigen

REPO = sys.argv[1]
NAME = "lm317_supply"
OUT = os.path.join(REPO, "hardware", "kicad")
os.makedirs(OUT, exist_ok=True)
FPLIB = r"D:\KICAD INSTALLATION\share\kicad\footprints"

TERM = "TerminalBlock_Phoenix:TerminalBlock_Phoenix_MKDS-1,5-2-5.08_1x02_P5.08mm_Horizontal"
TO220 = "Package_TO_SOT_THT:TO-220-3_Vertical"
D_H = "Diode_THT:D_DO-41_SOD81_P10.16mm_Horizontal"
D_V = "Diode_THT:D_DO-41_SOD81_P2.54mm_Vertical_CathodeUp"
CP8 = "Capacitor_THT:CP_Radial_D8.0mm_P3.50mm"
CP63 = "Capacitor_THT:CP_Radial_D6.3mm_P2.50mm"
R_H = "Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal"
POT = "Potentiometer_THT:Potentiometer_Bourns_3296W_Vertical"

# ref, symbol, value, footprint, pin->net, schematic (x,y), pcb (x, y, rotation)
PARTS = [
    dict(ref="J1", lib="Connector:Screw_Terminal_01x02", value="DC IN 15-24V", fp=TERM,
         pins={"1": "VIN_RAW", "2": "GND"}, at=(45.72, 63.5), pcb=(8, 6, 0)),
    dict(ref="D1", lib="Device:D", value="1N4007", fp=D_H,
         pins={"1": "VIN", "2": "VIN_RAW"}, at=(88.9, 50.8), pcb=(18.16, 20, 180)),
    dict(ref="C1", lib="Device:C_Polarized", value="470uF 35V", fp=CP8,
         pins={"1": "VIN", "2": "GND"}, at=(127, 76.2), pcb=(23, 28, 0)),
    dict(ref="U1", lib="Regulator_Linear:LM317_TO-220", value="LM317T", fp=TO220,
         pins={"1": "ADJ", "2": "VOUT", "3": "VIN"}, at=(177.8, 63.5), pcb=(40, 20, 180)),
    dict(ref="D2", lib="Device:D", value="1N4007", fp=D_V,
         pins={"1": "VIN", "2": "VOUT"}, at=(177.8, 101.6), pcb=(34.92, 15, 0)),
    dict(ref="R1", lib="Device:R", value="240R", fp=R_H,
         pins={"1": "VOUT", "2": "ADJ"}, at=(228.6, 50.8), pcb=(37.46, 11, 0)),
    dict(ref="RV1", lib="Device:R_Potentiometer", value="2k trimmer", fp=POT,
         pins={"1": "GND", "2": "ADJ", "3": "ADJ"}, at=(228.6, 88.9), pcb=(50.16, 28, 0)),
    dict(ref="C2", lib="Device:C_Polarized", value="100uF 25V", fp=CP63,
         pins={"1": "VOUT", "2": "GND"}, at=(254, 76.2), pcb=(52, 16, 0)),
    dict(ref="J2", lib="Connector:Screw_Terminal_01x02", value="DC OUT", fp=TERM,
         pins={"1": "VOUT", "2": "GND"}, at=(266.7, 101.6), pcb=(52, 6, 0)),
    dict(ref="#FLG01", lib="power:PWR_FLAG", value="PWR_FLAG", fp="",
         pins={"1": "VIN"}, at=(127, 50.8), pcb=None),
]

# ---------------- schematic ----------------
kigen.generate_sch(os.path.join(OUT, NAME + ".kicad_sch"), NAME, "LM317 Adjustable Supply 1.25-11.7 V",
                   "A", PARTS, "Demo project for MindModules Inner Circle")
kigen.write_project(os.path.join(OUT, NAME + ".kicad_pro"), NAME)

# ---------------- PCB ----------------
OX, OY = 100.0, 80.0
W, H = 66.0, 38.0
FM = pcbnew.FromMM


def P(x, y):
    return pcbnew.VECTOR2I(FM(OX + x), FM(OY + y))


board = pcbnew.BOARD()
nets = {}
for n in sorted({v for p in PARTS for v in p["pins"].values()}):
    ni = pcbnew.NETINFO_ITEM(board, n)
    board.Add(ni)
    nets[n] = ni

origins = {}
for p in PARTS:
    if p["pcb"] is None:
        continue
    lib, name = p["fp"].split(":")
    fp = pcbnew.FootprintLoad(os.path.join(FPLIB, lib + ".pretty"), name)
    fp.SetReference(p["ref"])
    fp.SetValue(p["value"])
    x, y, rot = p["pcb"]
    fp.SetPosition(P(x, y))
    if rot:
        fp.SetOrientationDegrees(rot)
    for pad in fp.Pads():
        net = p["pins"].get(pad.GetNumber())
        if net:
            pad.SetNet(nets[net])
    fp.Value().SetVisible(False)
    if p["ref"] in ("J1", "J2"):
        fp.Reference().SetVisible(False)   # terminals are labelled by silk text instead
    if p["ref"] == "C2":
        fp.Reference().SetPosition(P(x + 1.25, y + 7.2))
    board.Add(fp)
    origins[p["ref"]] = fp


def pad(ref, num):
    for pd in origins[ref].Pads():
        if pd.GetNumber() == str(num):
            pos = pd.GetPosition()
            return (round(pcbnew.ToMM(pos.x) - OX, 3), round(pcbnew.ToMM(pos.y) - OY, 3))
    raise KeyError((ref, num))


def track(net, pts, w):
    for a, b in zip(pts, pts[1:]):
        t = pcbnew.PCB_TRACK(board)
        t.SetStart(P(*a))
        t.SetEnd(P(*b))
        t.SetWidth(FM(w))
        t.SetLayer(pcbnew.F_Cu)
        t.SetNet(nets[net])
        board.Add(t)


def line(a, b, layer, width):
    s = pcbnew.PCB_SHAPE(board)
    s.SetShape(pcbnew.SHAPE_T_SEGMENT)
    s.SetStart(P(*a))
    s.SetEnd(P(*b))
    s.SetLayer(layer)
    s.SetWidth(FM(width))
    board.Add(s)


# board outline
for a, b in [((0, 0), (W, 0)), ((W, 0), (W, H)), ((W, H), (0, H)), ((0, H), (0, 0))]:
    line(a, b, pcbnew.Edge_Cuts, 0.1)

# mounting holes
for hx, hy in [(4, H - 4), (W - 4, H - 4)]:
    mh = pcbnew.FootprintLoad(os.path.join(FPLIB, "MountingHole.pretty"), "MountingHole_3.2mm_M3")
    mh.SetReference("H")
    mh.Reference().SetVisible(False)
    mh.Value().SetVisible(False)
    mh.SetPosition(P(hx, hy))
    board.Add(mh)

# routing (all on F.Cu, GND is a pour on B.Cu)
PW, SG = 1.0, 0.4
track("VIN_RAW", [pad("J1", 1), pad("D1", 2)], PW)
track("VIN", [pad("D1", 1), pad("U1", 3)], PW)
track("VIN", [(23, 20), pad("C1", 1)], PW)
track("VIN", [pad("D2", 1), (pad("D2", 1)[0], 20)], PW)
track("VOUT", [pad("U1", 2), (pad("U1", 2)[0], 6), pad("J2", 1)], PW)
track("VOUT", [pad("J2", 1), pad("C2", 1)], PW)
adj_x = pad("R1", 2)[0]
track("ADJ", [pad("U1", 1), (pad("U1", 1)[0], 16), (adj_x, 16)], SG)
track("ADJ", [pad("R1", 2), (adj_x, 28), pad("RV1", 2)], SG)
track("ADJ", [pad("RV1", 2), pad("RV1", 3)], SG)

# GND pour on B.Cu
z = pcbnew.ZONE(board)
z.SetLayer(pcbnew.B_Cu)
z.SetNet(nets["GND"])
ol = z.Outline()
ol.NewOutline()
for cx, cy in [(0.5, 0.5), (W - 0.5, 0.5), (W - 0.5, H - 0.5), (0.5, H - 0.5)]:
    v = P(cx, cy)
    ol.Append(v.x, v.y)
z.SetMinThickness(FM(0.25))
z.SetLocalClearance(FM(0.4))
z.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL)
board.Add(z)
pcbnew.ZONE_FILLER(board).Fill(board.Zones())


def text(s, x, y, size=1.2, layer=pcbnew.F_SilkS):
    t = pcbnew.PCB_TEXT(board)
    t.SetText(s)
    t.SetPosition(P(x, y))
    t.SetLayer(layer)
    t.SetTextSize(pcbnew.VECTOR2I(FM(size), FM(size)))
    t.SetTextThickness(FM(size / 6))
    board.Add(t)


text("LM317 SUPPLY v1.0", 33, H - 3, 1.3)
text("MindModules", 33, H - 1.2, 0.9)
text("IN+", 8, 1.4, 0.9)
text("GND", 13.08, 1.4, 0.9)
text("OUT+", 52, 1.4, 0.9)
text("GND", 57.08, 1.4, 0.9)

pcb_path = os.path.join(OUT, NAME + ".kicad_pcb")
board.Save(pcb_path)
print("saved", pcb_path)

"""Generate the 4S LiFePO4 BMS board: KiCad schematic + routed PCB.
Run with KiCad's bundled Python:  <KiCad>/bin/python.exe build_bms.py <repo_root>
"""
import os
import sys

import pcbnew

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import kigen
from router import Router

REPO = sys.argv[1]
NAME = "lifepo4_4s_bms"
OUT = os.path.join(REPO, "hardware", "kicad")
os.makedirs(OUT, exist_ok=True)
FPLIB = r"D:\KICAD INSTALLATION\share\kicad\footprints"

R_FP = "Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal"
RS_FP = "Resistor_THT:R_Axial_DIN0414_L11.9mm_D4.5mm_P15.24mm_Horizontal"
TO92 = "Package_TO_SOT_THT:TO-92_Inline"
TO220 = "Package_TO_SOT_THT:TO-220-3_Vertical"
DIP8 = "Package_DIP:DIP-8_W7.62mm"
DZ = "Diode_THT:D_DO-41_SOD81_P10.16mm_Horizontal"
CAP = "Capacitor_THT:C_Disc_D5.0mm_W2.5mm_P5.00mm"
TERM = "TerminalBlock_Phoenix:TerminalBlock_Phoenix_MKDS-1,5-2-5.08_1x02_P5.08mm_Horizontal"
HDR5 = "Connector_PinHeader_2.54mm:PinHeader_1x05_P2.54mm_Vertical"

W, H = 100.0, 94.0

# ------------------------------------------------------------------ parts
# each part: ref, lib, value, fp, pins{pad: net}, pcb=(x, y, rot) or None, optional unit
PARTS = []


def add(ref, lib, value, fp, pins, pcb=None, unit=1):
    PARTS.append(dict(ref=ref, lib=lib, value=value, fp=fp, pins=pins, pcb=pcb, unit=unit))


cells = ["GND", "C1P", "C2P", "C3P", "BPLUS"]
for i in range(4):
    lo, hi = cells[i], cells[i + 1]
    n = i + 1
    x0 = 9 + 23.5 * i
    add("R%d" % n, "Device:R", "4.42k 1%", R_FP, {"1": hi, "2": "REF%d" % n}, (x0, 15, 90))
    add("R%d" % (n + 4), "Device:R", "10k 1%", R_FP, {"1": "REF%d" % n, "2": lo}, (x0, 28, 90))
    add("R%d" % (n + 8), "Device:R", "18R", R_FP, {"1": hi, "2": "K%d" % n}, (x0 + 11, 15, 90))
    add("U%d" % n, "Reference_Voltage:TL431LP", "TL431", TO92,
        {"1": "REF%d" % n, "2": lo, "3": "K%d" % n}, (x0 + 11, 27.23, 270))

add("U5", "Comparator:LM2903", "LM393", DIP8, {"1": "GATE", "2": "VREF", "3": "UVDIV"}, (38, 62, 0), unit=1)
add("U5", "Comparator:LM2903", "LM393", DIP8, {"5": "OCTH", "6": "OCSENSE", "7": "GATE"}, None, unit=2)
add("U5", "Comparator:LM2903", "LM393", DIP8, {"4": "GND", "8": "BPLUS"}, None, unit=3)
add("U6", "Reference_Voltage:TL431LP", "TL431", TO92, {"1": "VREF", "2": "GND", "3": "VREF"}, (12, 61, 90))
add("Q1", "Transistor_FET:BUZ11", "IRLZ44N", TO220, {"1": "GATE", "2": "PMINUS", "3": "SRC"}, (50, 76, 0))
add("D1", "Device:D_Zener", "15V", DZ, {"1": "GATE", "2": "SRC"}, (34, 79, 0))
add("R13", "Device:R", "6.8k", R_FP, {"1": "BPLUS", "2": "VREF"}, (12, 42, 90))
add("R14", "Device:R", "34.8k 1%", R_FP, {"1": "BPLUS", "2": "UVDIV"}, (24, 42, 90))
add("R15", "Device:R", "10k 1%", R_FP, {"1": "UVDIV", "2": "GND"}, (24, 53, 90))
add("R16", "Device:R", "2.2M", R_FP, {"1": "GATE", "2": "UVDIV"}, (32, 42, 90))
add("R17", "Device:R", "100k", R_FP, {"1": "BPLUS", "2": "GATE"}, (40, 42, 90))
add("R18", "Device:R", "11.5k 1%", R_FP, {"1": "VREF", "2": "OCTH"}, (52, 42, 90))
add("R19", "Device:R", "1k 1%", R_FP, {"1": "OCTH", "2": "GND"}, (52, 53, 90))
add("R20", "Device:R", "1k", R_FP, {"1": "SRC", "2": "OCSENSE"}, (62, 42, 90))
add("R21", "Device:R", "0.02R 2W", RS_FP, {"1": "GND", "2": "SRC"}, (86, 56, 270))
add("C1", "Device:C", "100n", CAP, {"1": "VREF", "2": "GND"}, (19, 45, 90))
add("C2", "Device:C", "100n", CAP, {"1": "BPLUS", "2": "GND"}, (57, 62, 90))
add("C3", "Device:C", "100n", CAP, {"1": "OCSENSE", "2": "GND"}, (70, 45, 90))
add("J1", "Connector:Screw_Terminal_01x02", "PACK", TERM, {"1": "BPLUS", "2": "GND"}, (11, 84, 0))
add("J2", "Connector:Screw_Terminal_01x02", "LOAD / CHARGER", TERM, {"1": "PMINUS", "2": "BPLUS"}, (80, 84, 0))
add("J3", "Connector_Generic:Conn_01x05", "CELL TAPS", HDR5,
    {"1": "GND", "2": "C1P", "3": "C2P", "4": "C3P", "5": "BPLUS"}, (41, 4, 270))
add("#FLG01", "power:PWR_FLAG", "PWR_FLAG", "", {"1": "BPLUS"}, None)
add("#FLG02", "power:PWR_FLAG", "PWR_FLAG", "", {"1": "GND"}, None)

# schematic positions: simple grid, 9 columns
for i, p in enumerate(PARTS):
    p["at"] = (25.4 + 30.48 * (i % 9), 25.4 + 33.02 * (i // 9))

# ------------------------------------------------------------------ schematic
kigen.generate_sch(os.path.join(OUT, NAME + ".kicad_sch"), NAME, "4S LiFePO4 BMS: balancer and protection",
                   "A", PARTS, "Demo project for MindModules Inner Circle")
kigen.write_project(os.path.join(OUT, NAME + ".kicad_pro"), NAME)

# ------------------------------------------------------------------ PCB
FM = pcbnew.FromMM
board = pcbnew.BOARD()
nets = {}
for n in sorted({v for p in PARTS for v in p["pins"].values()}):
    ni = pcbnew.NETINFO_ITEM(board, n)
    board.Add(ni)
    nets[n] = ni


def P(x, y):
    return pcbnew.VECTOR2I(FM(x), FM(y))


# merge pins of all units per ref
pinmap = {}
for p in PARTS:
    pinmap.setdefault(p["ref"], {}).update(p["pins"])

fps = {}
for p in PARTS:
    if p["pcb"] is None or p["ref"] in fps:
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
        net = pinmap[p["ref"]].get(pad.GetNumber())
        if net:
            pad.SetNet(nets[net])
            if net == "GND" and p["fp"] == TO92:
                pad.SetLocalZoneConnection(pcbnew.ZONE_CONNECTION_FULL)
    fp.Value().SetVisible(False)
    board.Add(fp)
    fps[p["ref"]] = fp

# every J gets its reference hidden (labelled by silk text); keep others visible
for r in ("J1", "J2", "J3"):
    fps[r].Reference().SetVisible(False)


def pads_of(ref):
    out = {}
    for pd in fps[ref].Pads():
        pos = pd.GetPosition()
        sz = pd.GetSize()
        out[pd.GetNumber()] = (pcbnew.ToMM(pos.x), pcbnew.ToMM(pos.y), max(pcbnew.ToMM(sz.x), pcbnew.ToMM(sz.y)) / 2)
    return out


# bounding boxes for placement checks
for r in ("J1", "J2", "J3", "Q1", "U5", "R21", "U2"):
    bb = fps[r].GetBoundingBox(False)
    print("bbox", r, round(pcbnew.ToMM(bb.GetLeft()), 1), round(pcbnew.ToMM(bb.GetTop()), 1),
          round(pcbnew.ToMM(bb.GetRight()), 1), round(pcbnew.ToMM(bb.GetBottom()), 1))


def track(net, layer, pts, width):
    for a, b in zip(pts, pts[1:]):
        t = pcbnew.PCB_TRACK(board)
        t.SetStart(P(*a))
        t.SetEnd(P(*b))
        t.SetWidth(FM(width))
        t.SetLayer(layer)
        t.SetNet(nets[net])
        board.Add(t)


def via(net, x, y):
    v = pcbnew.PCB_VIA(board)
    v.SetPosition(P(x, y))
    v.SetWidth(FM(0.8))
    v.SetDrill(FM(0.4))
    v.SetNet(nets[net])
    v.SetViaType(pcbnew.VIATYPE_THROUGH)
    board.Add(v)


def line(a, b, layer, width):
    s = pcbnew.PCB_SHAPE(board)
    s.SetShape(pcbnew.SHAPE_T_SEGMENT)
    s.SetStart(P(*a))
    s.SetEnd(P(*b))
    s.SetLayer(layer)
    s.SetWidth(FM(width))
    board.Add(s)


for a, b in [((0, 0), (W, 0)), ((W, 0), (W, H)), ((W, H), (0, H)), ((0, H), (0, 0))]:
    line(a, b, pcbnew.Edge_Cuts, 0.1)

HOLES = [(4, 4), (W - 4, 4), (4, H - 4), (W - 4, H - 4)]
for hx, hy in HOLES:
    mh = pcbnew.FootprintLoad(os.path.join(FPLIB, "MountingHole.pretty"), "MountingHole_3.2mm_M3")
    mh.SetReference("H")
    mh.Reference().SetVisible(False)
    mh.Value().SetVisible(False)
    mh.SetPosition(P(hx, hy))
    board.Add(mh)

# ---------------------------------------------------------------- routing
rt = Router(W, H)
for hx, hy in HOLES:
    for L in (0, 1):
        rt.mark(L, hx, hy, 3.4, "HOLE")

netpads = {}
for ref, fp in fps.items():
    for num, (x, y, r) in pads_of(ref).items():
        net = pinmap[ref].get(num)
        if net is None:
            continue
        rt.add_pad(x, y, r, net)
        netpads.setdefault(net, []).append((x, y))

# power path: wide tracks placed by hand
J1p, J2p, Q1p, RSp = pads_of("J1"), pads_of("J2"), pads_of("Q1"), pads_of("R21")
BOT = H - 6.0
PW = 2.0
wide = []
# BPLUS: J1.1 down, along the bottom, up to J2.2
wide.append(("BPLUS", pcbnew.F_Cu, [J1p["1"][:2], (J1p["1"][0], BOT), (J2p["2"][0], BOT), J2p["2"][:2]], PW))
# SRC: Rs pad 2 to Q1 source
s2, qs = RSp["2"], Q1p["3"]
wide.append(("SRC", pcbnew.F_Cu, [s2[:2], (s2[0], qs[1]), qs[:2]], PW))
# PMINUS: Q1 drain to J2.1
qd, j21 = Q1p["2"], J2p["1"]
ymid = max(qd[1], qs[1]) + 4.5
wide.append(("PMINUS", pcbnew.F_Cu, [qd[:2], (qd[0], ymid), (j21[0], ymid), j21[:2]], PW))
for net, layer, pts, w in wide:
    L = 0
    rt.add_fixed_track(net, L, pts, w)
    track(net, layer, pts, w)

# tie J3 pin 1 (GND) into the pour region with a short route to a neighbouring GND pad
gnd_tie = [pads_of("J3")["1"][:2], pads_of("R5")["2"][:2]]
fails = []
order = sorted((n for n in netpads if n != "GND"), key=lambda n: (n not in ("C1P", "C2P", "C3P"), len(netpads[n]) > 6, sum(
    abs(a[0] - b[0]) + abs(a[1] - b[1]) for a, b in zip(netpads[n], netpads[n][1:]))))
for net in order + ["GND"]:
    f = rt.route_net(net, gnd_tie if net == "GND" else netpads[net])
    if f:
        fails.append((net, f))

for net, layer, a, b in rt.tracks:
    track(net, pcbnew.F_Cu if layer == 0 else pcbnew.B_Cu, [a, b], 0.4)
for net, x, y in rt.vias:
    via(net, x, y)

print("ROUTE FAILURES:", fails if fails else "none", "| tracks", len(rt.tracks), "vias", len(rt.vias))

# GND pour on B.Cu
z = pcbnew.ZONE(board)
z.SetLayer(pcbnew.B_Cu)
z.SetNet(nets["GND"])
ol = z.Outline()
ol.NewOutline()
for cx, cy in [(0.6, 0.6), (W - 0.6, 0.6), (W - 0.6, H - 0.6), (0.6, H - 0.6)]:
    v = P(cx, cy)
    ol.Append(v.x, v.y)
z.SetMinThickness(FM(0.25))
z.SetLocalClearance(FM(0.4))
z.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL)
board.Add(z)
pcbnew.ZONE_FILLER(board).Fill(board.Zones())


def text(s, x, y, size=1.1, layer=pcbnew.F_SilkS):
    t = pcbnew.PCB_TEXT(board)
    t.SetText(s)
    t.SetPosition(P(x, y))
    t.SetLayer(layer)
    t.SetTextSize(pcbnew.VECTOR2I(FM(size), FM(size)))
    t.SetTextThickness(FM(size / 6))
    board.Add(t)


text("LIFEPO4 4S BMS v1.0", 50, H - 2.2, 1.2)
text("MindModules", 50, H - 0.9, 0.8)
for ref, labels, dy in (("J1", {"1": "B+", "2": "B-"}, -7.0), ("J2", {"1": "P-", "2": "P+"}, -7.0),
                        ("J3", {"1": "B-", "2": "C1", "3": "C2", "4": "C3", "5": "B+"}, -2.3)):
    for num, lab in labels.items():
        px, py = pads_of(ref)[num][:2]
        text(lab, px, py + dy, 0.8 if ref == "J3" else 1.0)
text("PACK", 11, 91.2, 0.9)
text("LOAD / CHARGER", 82, 91.2, 0.9)
text("CELL TAPS", 49, 4, 0.9)

pcb_path = os.path.join(OUT, NAME + ".kicad_pcb")
board.Save(pcb_path)
print("saved", pcb_path)

"""Generate the two channel transistor relay driver: KiCad schematic + routed PCB.
Run with KiCad's bundled Python:  <KiCad>/bin/python.exe build_relay.py <repo_root>
"""
import os
import sys

import pcbnew

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import kigen
from router import Router

REPO = sys.argv[1]
NAME = "relay_driver"
OUT = os.path.join(REPO, "hardware", "kicad")
os.makedirs(OUT, exist_ok=True)
FPLIB = r"D:\KICAD INSTALLATION\share\kicad\footprints"

R_FP = "Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal"
D_FP = "Diode_THT:D_DO-35_SOD27_P7.62mm_Horizontal"
LED_FP = "LED_THT:LED_D3.0mm"
TO92 = "Package_TO_SOT_THT:TO-92_Inline_Wide"
RELAY = "Relay_THT:Relay_SPDT_SANYOU_SRD_Series_Form_C"
TERM2 = "TerminalBlock_Phoenix:TerminalBlock_Phoenix_MKDS-1,5-2-5.08_1x02_P5.08mm_Horizontal"
TERM3 = "TerminalBlock_Phoenix:TerminalBlock_Phoenix_MKDS-1,5-3-5.08_1x03_P5.08mm_Horizontal"
HDR4 = "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical"
CP_FP = "Capacitor_THT:CP_Radial_D6.3mm_P2.50mm"
C_FP = "Capacitor_THT:C_Disc_D5.0mm_W2.5mm_P5.00mm"

W, H = 64.0, 76.0
CH_X = [0.0, 32.0]          # x offset of each channel

PARTS = []


def add(ref, lib, value, fp, pins, pcb=None, unit=1):
    PARTS.append(dict(ref=ref, lib=lib, value=value, fp=fp, pins=pins, pcb=pcb, unit=unit))


for i, ox in enumerate(CH_X):
    n = i + 1
    coil, base, inp, leda = "COIL%d" % n, "BASE%d" % n, "IN%d" % n, "LEDA%d" % n
    com, no, nc = "COM%d" % n, "NO%d" % n, "NC%d" % n
    # relay pads: 1 COM, 2 and 5 coil, 3 NO, 4 NC
    add("K%d" % n, "Relay:SANYOU_SRD_Form_C", "SRD-05VDC-SL-C", RELAY,
        {"1": com, "2": "VCC", "3": no, "4": nc, "5": coil}, (ox + 9, 22, 0))
    add("J%d" % n, "Connector:Screw_Terminal_01x03", "RELAY %d" % n, TERM3,
        {"1": com, "2": nc, "3": no}, (ox + 5, 6, 0))
    add("D%d" % n, "Device:D", "1N4148", D_FP, {"1": "VCC", "2": coil}, (ox + 11, 35, 0))
    add("R%d" % (n + 4), "Device:R", "1k", R_FP, {"1": "VCC", "2": leda}, (ox + 11, 39.5, 0))
    add("D%d" % (n + 2), "Device:LED", "red 3mm", LED_FP, {"1": coil, "2": leda}, (ox + 24.54, 39.5, 180))
    add("Q%d" % n, "Transistor_BJT:Q_NPN_CBE", "BC337", TO92,
        {"1": coil, "2": base, "3": "GND"}, (ox + 9, 45.5, 0))
    add("R%d" % (2 * n - 1), "Device:R", "470R", R_FP, {"1": inp, "2": base}, (ox + 19.16, 51, 180))
    add("R%d" % (2 * n), "Device:R", "10k", R_FP, {"1": base, "2": "GND"}, (ox + 11.54, 55.5, 0))

add("J3", "Connector:Screw_Terminal_01x02", "5V IN", TERM2, {"1": "VCC", "2": "GND"}, (20, H - 7, 180))
add("J4", "Connector_Generic:Conn_01x04", "CONTROL", HDR4,
    {"1": "VCC", "2": "IN1", "3": "IN2", "4": "GND"}, (36, H - 6, 270))
add("C1", "Device:C_Polarized", "100uF 10V", CP_FP, {"1": "VCC", "2": "GND"}, (46, H - 7, 0))
add("C2", "Device:C", "100n", C_FP, {"1": "VCC", "2": "GND"}, (52, H - 14, 0))
add("#FLG01", "power:PWR_FLAG", "PWR_FLAG", "", {"1": "VCC"}, None)
add("#FLG02", "power:PWR_FLAG", "PWR_FLAG", "", {"1": "GND"}, None)

for i, p in enumerate(PARTS):
    p["at"] = (25.4 + 33.02 * (i % 8), 30.48 + 40.64 * (i // 8))

# ------------------------------------------------------------------ schematic
kigen.generate_sch(os.path.join(OUT, NAME + ".kicad_sch"), NAME, "Two channel transistor relay driver (5 V coil)",
                   "A", PARTS, "Demo project for MindModules Inner Circle")
kigen.write_project(os.path.join(OUT, NAME + ".kicad_pro"), NAME)

# ------------------------------------------------------------------ PCB
FM = pcbnew.FromMM
board = pcbnew.BOARD()
nets = {}
for nn in sorted({v for p in PARTS for v in p["pins"].values()}):
    ni = pcbnew.NETINFO_ITEM(board, nn)
    board.Add(ni)
    nets[nn] = ni


def P(x, y):
    return pcbnew.VECTOR2I(FM(x), FM(y))


pinmap = {p["ref"]: p["pins"] for p in PARTS}
fps = {}
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
        net = pinmap[p["ref"]].get(pad.GetNumber())
        if net:
            pad.SetNet(nets[net])
    fp.Value().SetVisible(False)
    board.Add(fp)
    fps[p["ref"]] = fp

for r in ("J1", "J2", "J3", "J4"):
    fps[r].Reference().SetVisible(False)
# references inside the body outline for axial parts, beside the part for the rest
for r, fp in fps.items():
    rf = fp.Reference()
    if r[0] in "RD" and not r.startswith("D3") and not r.startswith("D4") or r == "C2":
        a, b = [pd.GetPosition() for pd in fp.Pads()][:2]
        rf.SetPosition(pcbnew.VECTOR2I((a.x + b.x) // 2, (a.y + b.y) // 2))
        rf.SetTextSize(pcbnew.VECTOR2I(FM(0.8), FM(0.8)))
        rf.SetTextThickness(FM(0.12))
        rf.SetTextAngleDegrees(0)
    if r.startswith("Q"):
        pos = fp.GetPosition()
        rf.SetPosition(pcbnew.VECTOR2I(pos.x - FM(3.6), pos.y))
        rf.SetTextAngleDegrees(0)


def pads_of(ref):
    out = {}
    for pd in fps[ref].Pads():
        pos = pd.GetPosition()
        sz = pd.GetSize()
        out[pd.GetNumber()] = (round(pcbnew.ToMM(pos.x), 3), round(pcbnew.ToMM(pos.y), 3),
                               max(pcbnew.ToMM(sz.x), pcbnew.ToMM(sz.y)) / 2)
    return out


for r in sorted(fps):
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

HOLES = [(4, H - 4), (W - 4, H - 4)]
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
for ref in fps:
    for num, (x, y, r) in pads_of(ref).items():
        net = pinmap[ref].get(num)
        if net is None:
            continue
        rt.add_pad(x, y, r, net)
        netpads.setdefault(net, []).append((x, y))

# relay contact tracks: wide, on the top layer, placed by hand
CW = 1.5
wide = []
for i, ox in enumerate(CH_X):
    n = i + 1
    K, J = pads_of("K%d" % n), pads_of("J%d" % n)
    com, nc, no = K["1"][:2], K["4"][:2], K["3"][:2]
    jc, jnc, jno = J["1"][:2], J["2"][:2], J["3"][:2]
    wide.append(("COM%d" % n, [com, (jc[0], com[1]), jc], CW))
    wide.append(("NC%d" % n, [nc, (nc[0], 12.5), (jnc[0], 12.5), jnc], CW))
    wide.append(("NO%d" % n, [no, (ox + 26.5, no[1]), (ox + 26.5, 10.0), (jno[0], 10.0), jno], CW))
for net, pts, w in wide:
    rt.add_fixed_track(net, 0, pts, w)
    track(net, pcbnew.F_Cu, pts, w)

fails = []
signal = [nn for nn in netpads if nn != "GND" and not nn.startswith(("COM", "NC", "NO"))]
order = sorted(signal, key=lambda nn: (nn == "VCC", sum(abs(a[0] - b[0]) + abs(a[1] - b[1])
                                                       for a, b in zip(netpads[nn], netpads[nn][1:]))))
for net in order + ["GND"]:
    f = rt.route_net(net, netpads[net])
    if f:
        fails.append((net, f))

for net, layer, a, b in rt.tracks:
    track(net, pcbnew.F_Cu if layer == 0 else pcbnew.B_Cu, [a, b], 0.5 if net in ("VCC", "GND") else 0.4)
for net, x, y in rt.vias:
    via(net, x, y)

print("ROUTE FAILURES:", fails if fails else "none", "| tracks", len(rt.tracks), "vias", len(rt.vias))

# GND pour on B.Cu, kept away from the relay contact area
z = pcbnew.ZONE(board)
z.SetLayer(pcbnew.B_Cu)
z.SetNet(nets["GND"])
ol = z.Outline()
ol.NewOutline()
for cx, cy in [(0.6, 31.0), (W - 0.6, 31.0), (W - 0.6, H - 0.6), (0.6, H - 0.6)]:
    v = P(cx, cy)
    ol.Append(v.x, v.y)
z.SetMinThickness(FM(0.25))
z.SetLocalClearance(FM(0.4))
z.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL)
board.Add(z)
pcbnew.ZONE_FILLER(board).Fill(board.Zones())


def text(s, x, y, size=1.0, layer=pcbnew.F_SilkS):
    t = pcbnew.PCB_TEXT(board)
    t.SetText(s)
    t.SetPosition(P(x, y))
    t.SetLayer(layer)
    t.SetTextSize(pcbnew.VECTOR2I(FM(size), FM(size)))
    t.SetTextThickness(FM(size / 6))
    board.Add(t)


for n in (1, 2):
    J = pads_of("J%d" % n)
    for num, lab in (("1", "COM"), ("2", "NC"), ("3", "NO")):
        text(lab, J[num][0], 1.3, 0.8)
J3, J4 = pads_of("J3"), pads_of("J4")
text("+5V", J3["1"][0], H - 13.2, 0.8)
text("GND", J3["2"][0], H - 13.2, 0.8)
for num, lab in (("1", "5V"), ("2", "IN1"), ("3", "IN2"), ("4", "GND")):
    text(lab, J4[num][0], J4[num][1] + 2.3, 0.8)
text("RELAY DRIVER v1.0", 32, 35.2, 1.0)
text("MindModules", 32, 37.0, 0.8)
text("24V DC MAX ON CONTACTS", 32, 32.0, 0.8)

pcb_path = os.path.join(OUT, NAME + ".kicad_pcb")
board.Save(pcb_path)
print("saved", pcb_path)

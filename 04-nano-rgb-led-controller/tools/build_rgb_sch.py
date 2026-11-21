"""Generate the Nano RGB LED controller schematic (KiCad). Pure Python.
Usage: python build_rgb_sch.py <repo_root>
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import kigen

REPO = sys.argv[1]
NAME = "nano_rgb_controller"
OUT = os.path.join(REPO, "hardware", "kicad")
os.makedirs(OUT, exist_ok=True)

TERM2 = "TerminalBlock_Phoenix:TerminalBlock_Phoenix_MKDS-1,5-2-5.08_1x02_P5.08mm_Horizontal"
TERM4 = "TerminalBlock_Phoenix:TerminalBlock_Phoenix_MKDS-1,5-4-5.08_1x04_P5.08mm_Horizontal"
TO220 = "Package_TO_SOT_THT:TO-220-3_Vertical"
R_H = "Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal"
D_H = "Diode_THT:D_DO-41_SOD81_P10.16mm_Horizontal"
CP63 = "Capacitor_THT:CP_Radial_D6.3mm_P2.50mm"
NANO = "Module:Arduino_Nano"

parts = [
    dict(ref="U1", lib="MCU_Module:Arduino_Nano_v2.x", value="Arduino Nano", fp=NANO, nc=True,
         pins={"12": "PWM_R", "13": "PWM_G", "14": "PWM_B", "19": "POT_WIPER", "5": "BTN",
               "27": "+5V", "4": "GND", "29": "GND", "30": "VIN"}, at=(127.0, 101.6)),
    dict(ref="J1", lib="Connector:Screw_Terminal_01x02", value="12V IN", fp=TERM2,
         pins={"1": "+12V", "2": "GND"}, at=(40.64, 38.1)),
    dict(ref="D1", lib="Device:D", value="1N4007", fp=D_H,
         pins={"1": "VIN", "2": "+12V"}, at=(68.58, 38.1)),
    dict(ref="C1", lib="Device:C_Polarized", value="100uF 25V", fp=CP63,
         pins={"1": "+12V", "2": "GND"}, at=(40.64, 76.2)),
    dict(ref="J3", lib="Connector_Generic:Conn_01x03", value="Brightness pot (10k)",
         fp="Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical",
         pins={"1": "+5V", "2": "POT_WIPER", "3": "GND"}, at=(40.64, 127.0)),
    dict(ref="J4", lib="Connector_Generic:Conn_01x02", value="Mode button",
         fp="Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical",
         pins={"1": "BTN", "2": "GND"}, at=(40.64, 152.4)),
    dict(ref="J2", lib="Connector:Screw_Terminal_01x04", value="LED STRIP (+12V common)", fp=TERM4,
         pins={"1": "+12V", "2": "LED_R", "3": "LED_G", "4": "LED_B"}, at=(266.7, 63.5)),
    dict(ref="#FLG01", lib="power:PWR_FLAG", value="PWR_FLAG", fp="", pins={"1": "+12V"}, at=(91.44, 38.1)),
    dict(ref="#FLG02", lib="power:PWR_FLAG", value="PWR_FLAG", fp="", pins={"1": "GND"}, at=(91.44, 63.5)),
    dict(ref="#FLG03", lib="power:PWR_FLAG", value="PWR_FLAG", fp="", pins={"1": "VIN"}, at=(91.44, 88.9)),
]
for i, (ch, x) in enumerate([("R", 177.8), ("G", 203.2), ("B", 228.6)]):
    parts.append(dict(ref="R%d" % (i + 1), lib="Device:R", value="100R", fp=R_H,
                      pins={"1": "PWM_" + ch, "2": "GATE_" + ch}, at=(x, 50.8)))
    parts.append(dict(ref="R%d" % (i + 4), lib="Device:R", value="10k", fp=R_H,
                      pins={"1": "GATE_" + ch, "2": "GND"}, at=(x, 76.2 + 0)))
    parts.append(dict(ref="Q%d" % (i + 1), lib="Transistor_FET:BUZ11", value="IRLZ44N", fp=TO220,
                      pins={"1": "GATE_" + ch, "2": "LED_" + ch, "3": "GND"}, at=(x + 7.62, 114.3)))

kigen.generate_sch(os.path.join(OUT, NAME + ".kicad_sch"), NAME,
                   "Nano RGB LED Strip Controller (12 V, 3-channel PWM)", "A", parts,
                   "Demo project for MindModules Inner Circle")
kigen.write_project(os.path.join(OUT, NAME + ".kicad_pro"), NAME)
print("wrote schematic")

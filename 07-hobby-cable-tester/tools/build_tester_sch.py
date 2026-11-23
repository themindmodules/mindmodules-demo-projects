"""Generate the 8-wire cable tester schematic (KiCad). Pure Python.
Usage: python build_tester_sch.py <project_root>
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import kigen

ROOT = sys.argv[1]
NAME = "cable_tester"
OUT = os.path.join(ROOT, "hardware", "kicad")
os.makedirs(OUT, exist_ok=True)

DIP16 = "Package_DIP:DIP-16_W7.62mm_Socket"
R_H = "Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal"
D_H = "Diode_THT:D_DO-35_SOD27_P7.62mm_Horizontal"
LED5 = "LED_THT:LED_D5.0mm"
CDISC = "Capacitor_THT:C_Disc_D5.0mm_W2.5mm_P5.00mm"
HDR8 = "Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical"

# 74HC595 output pins QA..QH and 74HC165 parallel inputs D0..D7
Q595 = ["15", "1", "2", "3", "4", "5", "6", "7"]
D165 = ["11", "12", "13", "14", "3", "4", "5", "6"]

parts = [
    # Nano: D2 button, D3 pass LED, D4 fail LED, D9 load, D10 latch, D11 data, D12 read, D13 clock
    dict(ref="U1", lib="MCU_Module:Arduino_Nano_v2.x", value="Arduino Nano", fp="Module:Arduino_Nano", nc=True,
         pins={"5": "BTN", "6": "MCU_PASS", "7": "MCU_FAIL", "12": "RX_LOAD", "13": "LATCH",
               "14": "SER", "15": "RX_DATA", "16": "SCK", "27": "+5V", "4": "GND", "29": "GND"},
         at=(45.72, 101.6)),
    # Drive register: sends one high bit down the cable at a time
    dict(ref="U2", lib="74xx:74HC595", value="74HC595 (drive)", fp=DIP16,
         pins=dict({"14": "SER", "11": "SCK", "12": "LATCH", "10": "+5V", "13": "GND", "16": "+5V",
                    "8": "GND", "9": "SER2"}, **{q: "DRV_Q%d" % (i + 1) for i, q in enumerate(Q595)}),
         at=(96.52, 45.72)),
    # Display register: one LED per wire, chained after U2
    dict(ref="U3", lib="74xx:74HC595", value="74HC595 (LEDs)", fp=DIP16, nc=True,
         pins=dict({"14": "SER2", "11": "SCK", "12": "LATCH", "10": "+5V", "13": "GND", "16": "+5V",
                    "8": "GND"}, **{q: "LEDQ_%d" % (i + 1) for i, q in enumerate(Q595)}),
         at=(96.52, 165.1)),
    # Sense register: reads the far end of all 8 wires in one go
    dict(ref="U4", lib="74xx:74LS165", value="74HC165 (sense)", fp=DIP16, nc=True,
         pins=dict({"1": "RX_LOAD", "2": "SCK", "15": "GND", "10": "GND", "9": "RX_DATA", "16": "+5V",
                    "8": "GND"}, **{d: "FAR_%d" % (i + 1) for i, d in enumerate(D165)}),
         at=(96.52, 105.41)),
    dict(ref="J1", lib="Connector_Generic:Conn_01x08", value="NEAR END (cable side A)", fp=HDR8,
         pins={str(i + 1): "NEAR_%d" % (i + 1) for i in range(8)}, at=(241.3, 50.8)),
    dict(ref="J2", lib="Connector_Generic:Conn_01x08", value="FAR END (cable side B)", fp=HDR8,
         pins={str(i + 1): "FAR_%d" % (i + 1) for i in range(8)}, at=(241.3, 101.6)),
    dict(ref="SW1", lib="Switch:SW_Push", value="WIRES (count select)", fp="Button_Switch_THT:SW_PUSH_6mm",
         pins={"1": "BTN", "2": "GND"}, at=(25.4, 160.02)),
    dict(ref="#FLG01", lib="power:PWR_FLAG", value="PWR_FLAG", fp="", pins={"1": "GND"}, at=(20.32, 30.48)),
]

for i, x in enumerate([60.96, 73.66, 86.36]):
    parts.append(dict(ref="C%d" % (i + 1), lib="Device:C", value="100nF", fp=CDISC,
                      pins={"1": "+5V", "2": "GND"}, at=(x, 190.5)))

for i in range(8):
    x = 129.54 + i * 12.7
    n = i + 1
    # drive path: U2 output -> 220R -> 1N4148 -> near end connector
    parts.append(dict(ref="R%d" % n, lib="Device:R", value="220R", fp=R_H,
                      pins={"1": "DRV_Q%d" % n, "2": "DRV_D%d" % n}, at=(x, 25.4)))
    parts.append(dict(ref="D%d" % n, lib="Device:D", value="1N4148", fp=D_H,
                      pins={"2": "DRV_D%d" % n, "1": "NEAR_%d" % n}, at=(x, 45.72)))
    # sense path: 22k pulldown on each far end line
    parts.append(dict(ref="R%d" % (n + 8), lib="Device:R", value="22k", fp=R_H,
                      pins={"1": "FAR_%d" % n, "2": "GND"}, at=(x, 101.6)))
    # display: U3 output -> 1k -> LED -> GND
    parts.append(dict(ref="R%d" % (n + 16), lib="Device:R", value="1k", fp=R_H,
                      pins={"1": "LEDQ_%d" % n, "2": "LEDA_%d" % n}, at=(x, 147.32)))
    parts.append(dict(ref="D%d" % (n + 8), lib="Device:LED", value="LED yellow (wire %d)" % n, fp=LED5,
                      pins={"2": "LEDA_%d" % n, "1": "GND"}, at=(x, 172.72)))

for j, (sig, col) in enumerate([("PASS", "green"), ("FAIL", "red")]):
    x = 241.3 + j * 15.24
    parts.append(dict(ref="R%d" % (25 + j), lib="Device:R", value="1k", fp=R_H,
                      pins={"1": "MCU_" + sig, "2": "LED_" + sig}, at=(x, 147.32)))
    parts.append(dict(ref="D%d" % (17 + j), lib="Device:LED", value="LED %s (%s)" % (col, sig), fp=LED5,
                      pins={"2": "LED_" + sig, "1": "GND"}, at=(x, 172.72)))

kigen.generate_sch(os.path.join(OUT, NAME + ".kicad_sch"), NAME,
                   "8-Wire Cable Continuity Tester (USB 5 V)", "A", parts,
                   "Demo project for MindModules Inner Circle")
kigen.write_project(os.path.join(OUT, NAME + ".kicad_pro"), NAME)
print("wrote schematic")

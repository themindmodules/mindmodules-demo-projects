"""Generate one day of synthetic mains telemetry (1 sample per minute) for a small shop.

Appliances: fridge (compressor cycles), lights (on 09:00-21:00), a kettle (short bursts),
and one injected fault: from 14:20 the fridge compressor stops cycling and stays ON
with a higher current (a typical failing-compressor signature), plus a voltage sag at 18:05.

This is SYNTHETIC data for demo purposes only. Nothing here was measured from real hardware.
Usage:  python sim.py out.csv
"""
import csv
import math
import random
import sys

random.seed(42)  # reproducible demo


def minute_of_day_to_hhmm(m):
    return "%02d:%02d" % (m // 60, m % 60)


def generate():
    rows = []
    fridge_on = False
    fridge_timer = 0
    for m in range(24 * 60):
        # supply voltage: 230 V with slow drift and noise
        v = 230 + 3 * math.sin(m / 180.0) + random.gauss(0, 0.8)
        if 18 * 60 + 5 <= m < 18 * 60 + 12:
            v -= 28  # injected sag

        # fridge cycles: 12 min on, 25 min off (healthy)
        fault = m >= 14 * 60 + 20
        if fault:
            fridge_on = True
        else:
            fridge_timer -= 1
            if fridge_timer <= 0:
                fridge_on = not fridge_on
                fridge_timer = 12 if fridge_on else 25
        fridge_w = (150 if not fault else 235) + random.gauss(0, 4) if fridge_on else 2

        lights_w = 120 + random.gauss(0, 2) if 9 * 60 <= m < 21 * 60 else 0
        kettle_w = 1800 + random.gauss(0, 20) if m in (8 * 60 + 30, 8 * 60 + 31, 13 * 60, 13 * 60 + 1, 17 * 60 + 45) else 0
        base_w = 15 + random.gauss(0, 1)  # router, standby loads

        p = max(0.0, fridge_w + lights_w + kettle_w + base_w)
        i = p / v
        rows.append({"minute": m, "time": minute_of_day_to_hhmm(m),
                     "voltage_v": round(v, 2), "current_a": round(i, 3), "power_w": round(p, 1)})
    return rows


if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else "telemetry.csv"
    rows = generate()
    with open(out, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)
    print("wrote %d rows to %s" % (len(rows), out))

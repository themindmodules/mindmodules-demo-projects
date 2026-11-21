"""Detect anomalies in the telemetry and write dashboard/data.js.

Three simple, explainable detectors (good for a hackathon: you can explain each in one sentence):
  1. VOLTAGE  : outside 207-253 V (about +/-10 % of 230 V).
  2. SPIKE    : power more than 4 robust-sigmas above a 60-minute rolling median
                AND above 1000 W absolute (ignores normal small variations).
  3. STUCK-ON : power stays above 200 W for 90 minutes in a row while the rolling
                std stays tiny (a load that never cycles, like a failing compressor).
Usage:  python analyze.py telemetry.csv ../dashboard/data.js
"""
import csv
import json
import statistics
import sys


def load(path):
    with open(path) as f:
        return [{"minute": int(r["minute"]), "time": r["time"], "v": float(r["voltage_v"]),
                 "i": float(r["current_a"]), "p": float(r["power_w"])} for r in csv.DictReader(f)]


def analyze(rows):
    events = []
    P = [r["p"] for r in rows]

    # 1. voltage
    in_event = False
    for r in rows:
        bad = r["v"] < 207 or r["v"] > 253
        if bad and not in_event:
            events.append({"minute": r["minute"], "time": r["time"], "type": "VOLTAGE",
                           "detail": "%.1f V outside 207-253 V" % r["v"]})
        in_event = bad

    # 2. spikes vs rolling median
    W = 60
    for k in range(W, len(rows)):
        win = P[k - W:k]
        med = statistics.median(win)
        mad = statistics.median([abs(x - med) for x in win]) or 1.0
        z = (P[k] - med) / (1.4826 * mad)
        if z > 4 and P[k] > 1000 and (not events or events[-1]["type"] != "SPIKE" or k - events[-1]["minute"] > 5):
            events.append({"minute": rows[k]["minute"], "time": rows[k]["time"], "type": "SPIKE",
                           "detail": "%.0f W vs 60-min median %.0f W (short high-load burst)" % (P[k], med)})

    # 3. stuck on
    run = 0
    flagged = False
    for k, r in enumerate(rows):
        if r["p"] > 200:
            run += 1
        else:
            run = 0
            flagged = False
        if run >= 90 and not flagged:
            events.append({"minute": r["minute"], "time": r["time"], "type": "STUCK-ON",
                           "detail": "load above 200 W for 90 min without cycling"})
            flagged = True
    events.sort(key=lambda e: e["minute"])
    return events


if __name__ == "__main__":
    rows = load(sys.argv[1])
    events = analyze(rows)
    out = sys.argv[2] if len(sys.argv) > 2 else "data.js"
    payload = {"p": [r["p"] for r in rows], "v": [r["v"] for r in rows], "events": events}
    with open(out, "w") as f:
        f.write("window.GRIDWATCH = " + json.dumps(payload) + ";\n")
    print("events:")
    for e in events:
        print("  %s  %-9s %s" % (e["time"], e["type"], e["detail"]))

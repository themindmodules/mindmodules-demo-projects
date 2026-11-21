# GridWatch: a hackathon starter kit (energy anomaly detection)

A complete 24-hour hackathon package: problem statement, solution design, working demo
(simulator + anomaly detector + dashboard), pitch script, judge Q&A, team roles and an hour-by-hour plan.

**Idea in one line:** a small shop or home gets a cheap meter that tells you *what went wrong*
(a failing fridge compressor, a voltage sag, a surge), not just how many units you used.

> **Status:** the software demo runs end to end on synthetic data (see below). The hardware plan in
> `docs/HARDWARE_PLAN.md` is a design only; nothing here was built or measured on a real mains circuit.

## Run the demo (needs only Python 3, no packages)

```sh
cd simulator
python sim.py telemetry.csv                       # 1 day of synthetic data, 1 sample/min
python analyze.py telemetry.csv ../dashboard/data.js
```

Then open `dashboard/index.html` in a browser. Expected detections on the bundled seed:

```
08:30  SPIKE     short high-load burst (kettle)
13:00  SPIKE     short high-load burst (kettle)
15:40  STUCK-ON  load above 200 W for 90 min without cycling   <- the injected failing compressor
17:45  SPIKE     short high-load burst (kettle)
18:05  VOLTAGE   200.6 V outside 207-253 V                       <- the injected sag
```

## What is in the box

| File | Use |
|---|---|
| `simulator/sim.py` | Synthetic telemetry with two injected faults |
| `simulator/analyze.py` | Three explainable detectors (voltage band, spike, stuck-on) |
| `dashboard/index.html` | Zero-dependency chart page, works from `file://` |
| `docs/PROBLEM.md` | Problem statement and who has it |
| `docs/SOLUTION.md` | Architecture and why each part exists |
| `docs/PITCH.md` | 3-minute pitch, slide by slide |
| `docs/JUDGE_QA.md` | Hard questions judges ask, with honest answers |
| `docs/TIMELINE_24H.md` | Hour-by-hour plan for a 24 h hackathon |
| `docs/TEAM_ROLES.md` | Roles for a team of 3-4 |
| `docs/HARDWARE_PLAN.md` | ESP32 + current/voltage sensing plan, with mains-safety rules |

## How to adapt it

Swap the simulator for real readings (the CSV columns are `minute,time,voltage_v,current_a,power_w`),
keep `analyze.py` and the dashboard as they are, and you have a live demo.

## License

Personal and educational use for MindModules Inner Circle subscribers. See `LICENSE.md`.

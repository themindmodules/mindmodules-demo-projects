# RollCall: a hackathon starter kit (QR attendance with proxy detection)

A complete 24-hour hackathon package: problem statement, solution design, working demo
(scan simulator + rule engine + dashboard), a self-test, pitch script, judge Q&A, team roles,
an hour-by-hour plan and an optional low-voltage hardware plan.

**Idea in one line:** QR attendance is fast, but a static code gets photographed and shared in
the class group within a minute. RollCall shows a code that changes every 15 seconds, binds each
student to one phone, and tells the teacher exactly which scans look like a friend marking a friend.

> **Status:** the software demo runs end to end on synthetic data. `sim.py`, `analyze.py` and
> `check_rules.py` were run with Python 3.12 and the output below is copied from that run. The
> self-test passes 12 hand-made rule cases and 50 simulator seeds. The dashboard script was syntax
> checked and its data file loaded, but the page was not reviewed in every browser. No real students,
> phones or classrooms were used, and the hardware plan in `docs/HARDWARE_PLAN.md` was not built.

## Run the demo (needs only Python 3, no packages)

```sh
cd simulator
python sim.py scans.csv                 # one 60-student lecture, seed 12
python analyze.py scans.csv roster.csv ../dashboard/data.js truth.json
python check_rules.py                   # rule tests plus 50 random lectures
```

Then open `dashboard/index.html` in a browser. Expected output on the bundled seed:

```
EE201-L07: 59 scans, 60 students
  PRESENT 46  LATE 2  REVIEW 1  SUSPECT 6  ABSENT 5
flags:
  00:09  SHARED-DEVICE  S044         phone of S058 scanned for 2 students: S058 S044
  00:28  FOREIGN-DEVICE S044         phone is registered to S058
  00:37  DUPLICATE      S005         repeat scan 14 s after the first, ignored
  00:40  DUPLICATE      S012         repeat scan 2 s after the first, ignored
  00:44  NEW-DEVICE     S051         phone not registered (student's own: D-9EAB)
  00:50  DUPLICATE      S018         repeat scan 19 s after the first, ignored
  00:54  INVALID-TOKEN  S039         code from session EE201-L06
  01:57  SHARED-DEVICE  S002 S015    phone of S057 scanned for 3 students: S057 S002 S015
  02:15  FOREIGN-DEVICE S002         phone is registered to S057
  02:33  FOREIGN-DEVICE S015         phone is registered to S057
  02:59  STALE-TOKEN    S014         code shown at 01:45, scanned 74 s later
  03:43  DUPLICATE      S054         repeat scan 27 s after the first, ignored
  04:46  STALE-TOKEN    S025         code shown at 02:45, scanned 121 s later
  11:24  LATE           S007         first scan 84 s after close
  11:30  LATE           S036         first scan 90 s after close
check against injected truth:
  students in the room marked present: 49 of 49 (1 need a quick review)
  cheating attempts blocked: 6 of 6  missed: none
```

The six blocked cases are the ones the simulator injected: three proxy scans from two friends'
phones, two forwarded screenshots and one photo of last week's code. The one REVIEW is an honest
student on a new phone, which is a known false alarm the teacher clears in a few seconds.

## What is in the box

| File | Use |
|---|---|
| `simulator/common.py` | Session settings and the rotating HMAC token |
| `simulator/sim.py` | Synthetic lecture with known honest and cheating cases, writes `truth.json` |
| `simulator/analyze.py` | Seven explainable rules, final status per student, writes `data.js` |
| `simulator/check_rules.py` | Self-test: one case per rule plus 50 random lectures |
| `dashboard/index.html` | Zero-dependency page, works from `file://`, light and dark |
| `docs/PROBLEM.md` | Problem statement and who has it |
| `docs/SOLUTION.md` | Architecture, the rules, and their limits |
| `docs/PRIVACY.md` | What data is kept, for how long, and what is never collected |
| `docs/PITCH.md` | 3-minute pitch, slide by slide |
| `docs/JUDGE_QA.md` | Hard questions judges ask, with honest answers |
| `docs/TIMELINE_24H.md` | Hour-by-hour plan for a 24 h hackathon |
| `docs/TEAM_ROLES.md` | Roles for a team of 3 or 4 |
| `docs/HARDWARE_PLAN.md` | Optional USB-powered ESP32 code display, 5 V only |

## How to adapt it

The analyzer reads two plain CSV files. Point a real scanning page at a small server that appends
rows with the columns `ts_s,student_id,device_id,session_id,window,token`, keep `analyze.py` and
the dashboard as they are, and the demo becomes live. Change `WINDOW_S`, `GRACE_S` and `CLOSE_S`
in `common.py` to match your classroom.

## License

Personal and educational use for MindModules Inner Circle subscribers. See `LICENSE.md`.

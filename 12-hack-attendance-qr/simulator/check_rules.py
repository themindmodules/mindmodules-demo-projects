"""Self-test for the attendance rules. Run:  python check_rules.py

Part 1: small hand-made cases, one per rule.
Part 2: run the simulator for seeds 1 to 50 and check that every injected cheating
        case is blocked and every honest student in the room is marked.
"""
import csv
import json
import os
import subprocess
import sys
import tempfile

from analyze import analyze, evaluate, load_roster, load_scans
from common import DEMO_SECRET, SESSION_ID, WINDOW_S, make_token

HERE = os.path.dirname(os.path.abspath(__file__))
failures = []


def check(name, cond):
    print("%s  %s" % ("ok  " if cond else "FAIL", name))
    if not cond:
        failures.append(name)


def row(ts, student, device, window=None, session=SESSION_ID, token=None):
    if window is None:
        window = int(ts // WINDOW_S)
    if token is None:
        token = make_token(DEMO_SECRET, session, window)
    return {"ts": ts, "student": student, "device": device, "session": session, "window": window, "token": token}


ROSTER = {"S001": "D-A", "S002": "D-B", "S003": "D-C", "S004": "D-D"}


def run(rows):
    status, flags = analyze(rows, ROSTER)
    return status, [f["type"] for f in flags]


def part1():
    st, fl = run([row(20, "S001", "D-A")])
    check("clean scan is PRESENT, others ABSENT", st["S001"] == "PRESENT" and st["S002"] == "ABSENT" and not fl)

    st, fl = run([row(20, "S001", "D-A", token="deadbeef")])
    check("bad token is INVALID and SUSPECT", fl == ["INVALID-TOKEN"] and st["S001"] == "SUSPECT")

    st, fl = run([row(20, "S001", "D-A", session="EE201-L06")])
    check("old session code is INVALID", fl == ["INVALID-TOKEN"] and st["S001"] == "SUSPECT")

    st, fl = run([row(29.0, "S001", "D-A", window=1)])
    check("token 14 s old (inside one window) is accepted", st["S001"] == "PRESENT" and not fl)

    st, fl = run([row(44.0, "S001", "D-A", window=1)])
    check("token 29 s old (inside grace) is accepted", st["S001"] == "PRESENT" and not fl)

    st, fl = run([row(46.0, "S001", "D-A", window=1)])
    check("token 31 s old is STALE", fl == ["STALE-TOKEN"] and st["S001"] == "SUSPECT")

    st, fl = run([row(20, "S001", "D-A"), row(35, "S001", "D-A")])
    check("double scan is one DUPLICATE, still PRESENT", fl == ["DUPLICATE"] and st["S001"] == "PRESENT")

    st, fl = run([row(20, "S001", "D-NEW")])
    check("unregistered phone gives REVIEW", fl == ["NEW-DEVICE"] and st["S001"] == "REVIEW")

    st, fl = run([row(20, "S001", "D-A"), row(30, "S002", "D-A")])
    check("friend's phone: FOREIGN-DEVICE and SHARED-DEVICE",
          "FOREIGN-DEVICE" in fl and "SHARED-DEVICE" in fl and st["S002"] == "SUSPECT" and st["S001"] == "PRESENT")

    st, fl = run([row(20, "S001", "D-X"), row(30, "S002", "D-X")])
    check("one unregistered phone for two students: both SUSPECT",
          "SHARED-DEVICE" in fl and st["S001"] == "SUSPECT" and st["S002"] == "SUSPECT")

    st, fl = run([row(20, "S002", "D-A", token="00000000"), row(25, "S002", "D-B")])
    check("a bad row then a clean row ends PRESENT", st["S002"] == "PRESENT")

    st, fl = run([row(650, "S003", "D-C")])
    check("scan after close is LATE", fl == ["LATE"] and st["S003"] == "LATE")


def part2():
    tmp = tempfile.mkdtemp(prefix="rc_", dir=HERE)
    try:
        worst = None
        for seed in range(1, 51):
            scans = os.path.join(tmp, "scans.csv")
            subprocess.run([sys.executable, os.path.join(HERE, "sim.py"), scans, str(seed)],
                           check=True, capture_output=True)
            status, _ = analyze(load_scans(scans), load_roster(os.path.join(tmp, "roster.csv")))
            with open(os.path.join(tmp, "truth.json")) as f:
                e = evaluate(status, json.load(f)["students"])
            good = e["in_room_marked"] == e["in_room_total"] and e["cheat_blocked"] == e["cheat_total"]
            if not good and worst is None:
                worst = (seed, e)
        check("seeds 1-50: all honest students marked, all cheating cases blocked", worst is None)
        if worst:
            print("     first bad seed:", worst)
    finally:
        for name in os.listdir(tmp):
            os.remove(os.path.join(tmp, name))
        os.rmdir(tmp)


if __name__ == "__main__":
    part1()
    part2()
    print("\n%s" % ("all checks passed" if not failures else "%d check(s) failed" % len(failures)))
    sys.exit(1 if failures else 0)

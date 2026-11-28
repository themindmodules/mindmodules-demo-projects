"""Generate one synthetic lecture of QR attendance scans, with known cheating cases.

Each scan row is what a student's phone would send to the server:
    ts_s, student_id, device_id, session_id, window, token
ts_s is the time the server received it, in seconds from the start of the lecture.

Injected cases (60 students):
    normal      honest scan from the student's own phone
    double      honest student who scans twice by accident
    newphone    honest student on a phone that is not registered yet
    late        honest student who arrives after scanning closes
    absent      not in the room, no scan at all
    proxied     not in the room, a friend scans for them on the friend's phone
    stale       not in the room, scans a screenshot forwarded by a friend
    forged      not in the room, reuses a code from last week's lecture
    (friends)   the honest students whose phones were used for the proxied ones

Usage:  python sim.py scans.csv [seed]
Also writes roster.csv and truth.json in the same folder as scans.csv.
"""
import csv
import json
import os
import random
import sys

from common import CLOSE_S, DEMO_SECRET, LATE_END_S, SESSION_ID, WINDOW_S, make_token

N_STUDENTS = 60
OLD_SESSION = "EE201-L06"


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "scans.csv"
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else 12
    rng = random.Random(seed)
    folder = os.path.dirname(os.path.abspath(out))

    ids = ["S%03d" % k for k in range(1, N_STUDENTS + 1)]
    used = set()

    def new_device():
        while True:
            d = "D-%04X" % rng.randrange(0x10000)
            if d not in used:
                used.add(d)
                return d

    registered = {s: new_device() for s in ids}

    pool = ids[:]
    rng.shuffle(pool)

    def take(n):
        return sorted(pool.pop() for _ in range(n))

    groups = {
        "absent": take(5), "proxied": take(3), "friend": take(2), "stale": take(2),
        "forged": take(1), "late": take(2), "double": take(4), "newphone": take(1),
    }
    groups["normal"] = sorted(pool)

    rows = []

    def arrival():
        # most students scan in the first few minutes, a few drift in later
        return 5 + min(rng.expovariate(1 / 75.0), CLOSE_S - 40)

    def scan(student, device, t_scan, window=None, session=SESSION_ID):
        if window is None:
            window = int(t_scan // WINDOW_S)
        lag = rng.uniform(0.3, 4.0)          # camera decode plus network delay
        rows.append({"ts_s": round(t_scan + lag, 1), "student_id": student, "device_id": device,
                     "session_id": session, "window": window,
                     "token": make_token(DEMO_SECRET, session, window)})

    for s in groups["normal"]:
        scan(s, registered[s], arrival())
    for s in groups["double"]:
        t = arrival()
        scan(s, registered[s], t)
        scan(s, registered[s], t + rng.uniform(4, 40))
    for s in groups["newphone"]:
        scan(s, new_device(), arrival())
    for s in groups["late"]:
        scan(s, registered[s], rng.uniform(CLOSE_S + 15, LATE_END_S - 10))

    # friend 1 scans for two absent classmates, friend 2 for one
    f1, f2 = groups["friend"]
    p1, p2, p3 = groups["proxied"]
    t = arrival()
    scan(f1, registered[f1], t)
    scan(p1, registered[f1], t + rng.uniform(8, 20))
    scan(p2, registered[f1], t + rng.uniform(25, 45))
    t = arrival()
    scan(f2, registered[f2], t)
    scan(p3, registered[f2], t + rng.uniform(8, 25))

    # a screenshot sent to a chat group, scanned at home a minute or two later
    for s in groups["stale"]:
        w = int(arrival() // WINDOW_S)
        scan(s, registered[s], w * WINDOW_S + rng.uniform(70, 150), window=w)

    # a photo of last week's code
    for s in groups["forged"]:
        scan(s, registered[s], arrival(), window=rng.randrange(0, 40), session=OLD_SESSION)

    rows.sort(key=lambda r: r["ts_s"])

    with open(out, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=["ts_s", "student_id", "device_id", "session_id", "window", "token"])
        w.writeheader()
        w.writerows(rows)
    with open(os.path.join(folder, "roster.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["student_id", "registered_device"])
        for s in ids:
            w.writerow([s, registered[s]])

    present_cases = {"normal", "double", "newphone", "late", "friend"}
    truth = {}
    for case, members in groups.items():
        for s in members:
            truth[s] = {"case": case, "in_room": case in present_cases}
    with open(os.path.join(folder, "truth.json"), "w") as f:
        json.dump({"seed": seed, "students": dict(sorted(truth.items()))}, f, indent=1)

    print("wrote %d scans for %d students (seed %d)" % (len(rows), N_STUDENTS, seed))
    for case in ["proxied", "stale", "forged", "absent", "late", "double", "newphone", "friend"]:
        print("  %-9s %s" % (case, " ".join(groups[case])))


if __name__ == "__main__":
    main()

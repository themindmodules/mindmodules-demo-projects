"""Decide who was present and flag duplicate and proxy scans. Writes dashboard/data.js.

Rules, checked in this order for every scan row (each one fits in a sentence on a slide):
  1. INVALID-TOKEN   wrong session, or the token does not match the window. Row rejected.
  2. STALE-TOKEN     token is real but older than one window plus grace (30 s). Row rejected.
                     This is what a forwarded screenshot looks like.
  3. FOREIGN-DEVICE  phone is registered to a different student. Row rejected.
  4. NEW-DEVICE      phone is not registered to anyone. Row accepted, student goes to REVIEW.
  5. DUPLICATE       student already has an accepted scan. Row ignored, no penalty.
  6. LATE            first accepted scan came after scanning closed.
After all rows:
  7. SHARED-DEVICE   one phone scanned for more than one student in this lecture.
                     Every student on it except the phone's owner goes to SUSPECT.

Final status per student:
  PRESENT  clean accepted scan before close
  LATE     clean accepted scan after close
  REVIEW   accepted, but on an unregistered phone (teacher checks in person)
  SUSPECT  only rejected scans, or scanned from a shared phone (not marked present)
  ABSENT   no scan at all

Usage:  python analyze.py scans.csv roster.csv ../dashboard/data.js [truth.json]
"""
import csv
import json
import sys
from collections import defaultdict

from common import CLOSE_S, DEMO_SECRET, GRACE_S, SESSION_ID, WINDOW_S, make_token, mmss

HARD = {"INVALID-TOKEN", "STALE-TOKEN", "FOREIGN-DEVICE", "SHARED-DEVICE"}


def load_scans(path):
    with open(path) as f:
        return [{"ts": float(r["ts_s"]), "student": r["student_id"], "device": r["device_id"],
                 "session": r["session_id"], "window": int(r["window"]), "token": r["token"]}
                for r in csv.DictReader(f)]


def load_roster(path):
    with open(path) as f:
        return {r["student_id"]: r["registered_device"] for r in csv.DictReader(f)}


def analyze(scans, roster, session_id=SESSION_ID, close_s=CLOSE_S, secret=DEMO_SECRET):
    owner = {d: s for s, d in roster.items()}
    flags = []
    accepted = {}                      # student -> first accepted scan
    review = set()
    rejected = defaultdict(list)       # student -> hard flag types
    on_device = defaultdict(list)      # device -> students it scanned for (token-valid rows)

    def flag(scan, kind, detail):
        flags.append({"ts": scan["ts"], "time": mmss(scan["ts"]), "type": kind,
                      "student": scan["student"], "device": scan["device"], "detail": detail})

    for sc in sorted(scans, key=lambda r: r["ts"]):
        s, dev = sc["student"], sc["device"]
        if sc["session"] != session_id or sc["token"] != make_token(secret, session_id, sc["window"]):
            why = "code from session %s" % sc["session"] if sc["session"] != session_id else "token does not match window"
            flag(sc, "INVALID-TOKEN", why)
            rejected[s].append("INVALID-TOKEN")
            continue
        age = sc["ts"] - sc["window"] * WINDOW_S
        if age > WINDOW_S + GRACE_S or age < -2:
            flag(sc, "STALE-TOKEN", "code shown at %s, scanned %d s later" % (mmss(sc["window"] * WINDOW_S), age))
            rejected[s].append("STALE-TOKEN")
            continue
        if s not in on_device[dev]:
            on_device[dev].append(s)
        reg_owner = owner.get(dev)
        if reg_owner is not None and reg_owner != s:
            flag(sc, "FOREIGN-DEVICE", "phone is registered to %s" % reg_owner)
            rejected[s].append("FOREIGN-DEVICE")
            continue
        if s in accepted:
            flag(sc, "DUPLICATE", "repeat scan %d s after the first, ignored" % (sc["ts"] - accepted[s]["ts"]))
            continue
        if reg_owner is None:
            flag(sc, "NEW-DEVICE", "phone not registered (student's own: %s)" % roster.get(s, "none"))
            review.add(s)
        accepted[s] = sc
        if sc["ts"] > close_s:
            flag(sc, "LATE", "first scan %d s after close" % (sc["ts"] - close_s))

    shared = set()
    for dev, studs in on_device.items():
        if len(studs) > 1:
            keep = owner.get(dev)
            others = [x for x in studs if x != keep]
            flags.append({"ts": accepted[keep]["ts"] if keep in accepted else 0,
                          "time": mmss(accepted[keep]["ts"]) if keep in accepted else "--:--",
                          "type": "SHARED-DEVICE", "student": " ".join(others), "device": dev,
                          "detail": "phone of %s scanned for %d students: %s" % (keep or "nobody", len(studs), " ".join(studs))})
            shared.update(others)

    status = {}
    for s in sorted(roster):
        if s in shared or (s not in accepted and rejected[s]):
            status[s] = "SUSPECT"
        elif s in accepted:
            status[s] = "REVIEW" if s in review else ("LATE" if accepted[s]["ts"] > close_s else "PRESENT")
        else:
            status[s] = "ABSENT"
    flags.sort(key=lambda e: e["ts"])
    return status, flags


def evaluate(status, truth):
    """Compare with the injected ground truth. Marked = PRESENT, LATE or REVIEW."""
    marked = {"PRESENT", "LATE", "REVIEW"}
    out = {"in_room_marked": 0, "in_room_total": 0, "in_room_review": 0,
           "cheat_total": 0, "cheat_blocked": 0, "missed": []}
    for s, t in truth.items():
        if t["in_room"]:
            out["in_room_total"] += 1
            out["in_room_marked"] += status[s] in marked
            out["in_room_review"] += status[s] == "REVIEW"
        elif t["case"] != "absent":
            out["cheat_total"] += 1
            if status[s] in marked:
                out["missed"].append(s)
            else:
                out["cheat_blocked"] += 1
    return out


def main():
    scans = load_scans(sys.argv[1])
    roster = load_roster(sys.argv[2])
    out = sys.argv[3] if len(sys.argv) > 3 else "data.js"
    status, flags = analyze(scans, roster)

    counts = defaultdict(int)
    for v in status.values():
        counts[v] += 1
    bins = [{"t": b * 30, "ok": 0, "bad": 0} for b in range(26)]
    rejected_ts = {(f["ts"], f["student"]) for f in flags if f["type"] in HARD - {"SHARED-DEVICE"}}
    for sc in scans:
        b = min(int(sc["ts"] // 30), len(bins) - 1)
        bins[b]["bad" if (sc["ts"], sc["student"]) in rejected_ts else "ok"] += 1

    payload = {"session": SESSION_ID, "close_s": CLOSE_S, "scans": len(scans), "counts": dict(counts),
               "status": status, "flags": flags, "bins": bins}
    if len(sys.argv) > 4:
        with open(sys.argv[4]) as f:
            truth = json.load(f)["students"]
        payload["eval"] = evaluate(status, truth)
    with open(out, "w") as f:
        f.write("window.ROLLCALL = " + json.dumps(payload) + ";\n")

    print("%s: %d scans, %d students" % (SESSION_ID, len(scans), len(status)))
    print("  " + "  ".join("%s %d" % (k, counts[k]) for k in ["PRESENT", "LATE", "REVIEW", "SUSPECT", "ABSENT"]))
    print("flags:")
    for f in flags:
        print("  %s  %-14s %-12s %s" % (f["time"], f["type"], f["student"], f["detail"]))
    if "eval" in payload:
        e = payload["eval"]
        print("check against injected truth:")
        print("  students in the room marked present: %d of %d (%d need a quick review)"
              % (e["in_room_marked"], e["in_room_total"], e["in_room_review"]))
        print("  cheating attempts blocked: %d of %d  missed: %s"
              % (e["cheat_blocked"], e["cheat_total"], " ".join(e["missed"]) or "none"))


if __name__ == "__main__":
    main()

# Judge Q&A (prepare honest answers)

**Is this real data?**
No. The demo uses a simulator with known honest and cheating cases, and `analyze.py` compares its
decisions with that ground truth. Say this before they ask.

**Can a student still cheat?**
Yes, with effort. A friend in the room can relay the code over a live video call within 30 seconds,
and the absent student scans it on their own registered phone. We block the cheap tricks (screenshots
in a group, old codes, one phone for many) and say openly that a determined pair can beat it. For
exam-critical lectures, add an in-room check like a Bluetooth beacon or a random two-name roll call.

**Why not GPS?**
Indoor GPS is off by tens of metres, fake-GPS apps are one download away, and location is far more
personal data than attendance needs. See `docs/PRIVACY.md`.

**What if a student's phone dies or they get a new one?**
A new phone scans as NEW-DEVICE: the student is marked as REVIEW, not rejected. The teacher confirms
in person and re-registers the phone. No phone at all: the teacher marks them by hand, as today.

**Why 15 seconds?**
Short enough that a forwarded photo is stale by the time it reaches the group, long enough that the
back row can focus the camera. It is a setting in `common.py`.

**How do you know the thresholds work?**
On synthetic data only. `check_rules.py` runs 50 random lectures and checks every injected case.
Real scan delays need to be measured in a pilot before trusting the 30 s limit.

**What if two siblings share a phone?**
It flags as SHARED-DEVICE. That is correct behaviour: the teacher sees it and decides. Policy for
families with one phone is the institute's call, not the software's.

**Business model?**
Do not invent numbers. Options: a free tier for single teachers, a paid plan for institutes with
reports and integration. Say these are hypotheses to test in a pilot.

**What did you build in 24 hours versus plan?**
Answer exactly: the rule engine, simulator, self-test and dashboard are built and run on simulated
data. The scanning web page and hardware display are planned (or say what you actually built).

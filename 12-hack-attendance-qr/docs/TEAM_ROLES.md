# Team roles (3 or 4 people)

| Role | Owns | Hands off to |
|---|---|---|
| **Backend / rules** | `common.py`, `analyze.py`, the self-test, the small server | Frontend (`data.js` and the API) |
| **Simulation / testing** | `sim.py`, the injected cases, trying to cheat the system | Backend (new rules or fixes) |
| **Frontend / demo** | Dashboard, code display page, scanning page, backup video | Pitch lead |
| **Pitch / story** | Teacher interviews, slides, script, judge Q&A, privacy answer | Everyone (rehearsal) |

Team of 3: the simulation person also owns the pitch. Team of 2: drop the scanning page and the
hardware, and run the demo on simulated data with a clear statement of what is planned.

The simulation person has one extra job: act as the cheater. Every new trick they find either
becomes a test case in `check_rules.py` or a line in the limitations list.

## Check-in questions (every 3 hours)

1. What works end to end right now?
2. What is blocking you?
3. What can we cut?

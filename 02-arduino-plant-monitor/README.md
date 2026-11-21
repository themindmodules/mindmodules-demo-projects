# Arduino Plant Monitor with Safe Auto-Watering

Reads a capacitive soil moisture sensor and a light sensor, logs CSV over serial, and waters
the plant with a small pump through a MOSFET. Built for a beginner who wants a project that
is actually useful and has real safety limits.

> **Status:** reference design. The control logic compiles cleanly (`firmware/test/check_syntax.sh`).
> The unit tests in `firmware/test/logic_check.cpp` are included but were **not executed** in this
> package (no host compiler was available). Run them yourself with the command in that file.
> Not tested on real hardware or with a real plant.

## What you get

| Path | Contents |
|---|---|
| `firmware/plant_monitor/plant_monitor.ino` | The sketch (no external libraries) |
| `firmware/plant_monitor/plant_logic.h` | Control logic with no Arduino calls, so it is testable |
| `firmware/test/` | Unit tests and a syntax-check script |
| `docs/build-guide.md` | Wiring, calibration, first run |
| `docs/safety.md` | Why the pump has hard limits, and what can go wrong |
| `docs/bom.csv` | Parts list |
| `hardware/wiring.svg` | Circuit / wiring diagram |

## How it behaves

- Soil sensor is averaged over 8 readings, converted to 0-100 %.
- **Hysteresis:** pump turns on below 30 %, off above 45 %, so it does not chatter.
- **Max run time:** a single watering never lasts more than 10 s.
- **Lockout:** at least 30 minutes between waterings.
- Pump is forced OFF at boot. Holding the button at boot enters calibration mode with the pump disabled.
- Serial output at 115200 baud: `ms,raw,moisture_pct,light_raw,pump`

## Quick start

1. Wire it as in `hardware/wiring.svg`.
2. Hold the button, power up, and note the raw value in dry air and in a glass of water.
3. Put those two numbers in `cfg.dryRaw` and `cfg.wetRaw` in the sketch.
4. Upload and open the Serial Monitor.

Full steps are in `docs/build-guide.md`.

## License

Personal and educational use for MindModules Inner Circle subscribers. See `LICENSE.md`.

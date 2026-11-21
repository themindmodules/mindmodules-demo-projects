# Build guide

## 1. Wiring

| Part | Connects to |
|---|---|
| Capacitive soil sensor VCC / GND / AOUT | 5V / GND / A0 |
| LDR | One leg to 5V, other leg to A1 |
| 10 kohm resistor | A1 to GND (forms the LDR divider) |
| MOSFET gate | D6 through 100 ohm; 100 kohm pulldown from gate to GND |
| MOSFET source | GND (common with the Arduino GND) |
| MOSFET drain | Pump negative |
| Pump positive | Pump supply positive (see below) |
| Flyback diode (1N4007) | Cathode to pump positive, anode to pump negative (across the pump) |
| Button | D2 to GND (internal pull-up is used) |

**Pump supply:** use a separate supply that matches your pump (for example 5 V or 6 V),
with its ground joined to the Arduino ground. Do not power a pump from the Arduino 5 V pin.

**MOSFET choice:** it must be *logic-level*, meaning fully on at 5 V gate drive (for example
IRLZ44N, or a small module based on one). A plain IRF540 may not switch fully at 5 V.

## 2. Calibration

1. Hold the button and power the board. Serial prints `raw=...` twice a second. The pump stays off.
2. Sensor in dry air: note the value (example 620). That is `dryRaw`.
3. Sensor tip in a glass of water up to the line on the sensor: note the value (example 300). That is `wetRaw`.
4. Edit `cfg` in `plant_monitor.ino`, upload.

Capacitive sensors read higher when dry. If yours reads lower when dry, swap the two numbers.

## 3. First run (no plant yet)

1. Pump disconnected. Watch the serial log with the sensor dry: `pump` should go to 1 and the LED should light.
2. After 10 s the pump value must return to 0 even though the sensor is still dry.
3. It must not turn on again for 30 minutes. To test sooner, temporarily set `lockoutMs` to 20 s.
4. Connect the pump with a cup of water and repeat.

## 4. Tuning

| Want | Change |
|---|---|
| Water earlier or later | `onBelowPct` |
| More water per run | `offAbovePct` or `maxPumpMs` |
| Slower watering cycle | `lockoutMs` |

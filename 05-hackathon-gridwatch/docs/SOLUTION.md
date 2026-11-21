# Solution design

```
 [mains] -> [isolated current + voltage sensing module] -> [ESP32] --Wi-Fi/MQTT--> [small server]
                                                                                      |
                                                              analyze.py (3 detectors) -> alerts
                                                                                      |
                                                                           dashboard + phone notification
```

## Parts and why

| Part | Why it exists |
|---|---|
| Isolated sensing module | Keeps the mains side separated from the microcontroller side (safety) |
| ESP32 | Wi-Fi built in, cheap, samples once a minute with ease |
| Server (laptop or Raspberry Pi) | Runs the detectors; at a hackathon this can be your laptop |
| Dashboard | The part judges actually look at; keep it clean |

## The three detectors (explainable on purpose)

1. **Voltage band:** flag readings outside 207-253 V (about +/-10 % of a 230 V supply).
2. **Spike:** power far above the recent 60-minute median *and* above 1000 W. Median/MAD is used instead
   of mean/std so one spike does not hide the next.
3. **Stuck-on:** load stays above 200 W for 90 minutes without cycling. Catches a compressor that
   never switches off.

Why not machine learning? With one day of data from one site there is nothing to train on, and judges
will ask. Simple rules you can explain beat a model you cannot defend. The roadmap slide can say:
"after a few weeks of data per site, learn each appliance's normal duty cycle".

## Limitations (say these out loud in the pitch)

- Thresholds are fixed and tuned on synthetic data.
- A kettle burst is flagged as a spike; real systems would learn known appliances.
- One measurement point per site cannot separate two appliances that run together.

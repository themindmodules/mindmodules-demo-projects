# Bring-up and test plan

This plan has not been run on hardware. It lists what the code and the calculations predict.
Do the steps in order. The first ones need no motor, so a wiring or pin mistake cannot damage anything.

## Without the motor

| # | Step | Expected result |
|---|---|---|
| 1 | Flash, open a terminal at 115200 8N1, press reset | Banner with `1320 counts per rev, loop 100 Hz, PWM 20000 Hz`, then the CSV header |
| 2 | Watch the log for a few seconds | Rows at 10 per second, `ms` going up by about 100, everything else 0 |
| 3 | Type `help` and `status` | Command list, then `# mode off ... kp 0.00270 ki 0.0330 fault none` |
| 4 | Scope on PB4 and PB5, send `open 50` | PB4 steady high, PB5 a 20 kHz square wave at 50 percent (slow decay) |
| 5 | Send `open -25` | PB4 a 20 kHz wave high 75 percent of the time, PB5 steady high |
| 6 | Wait 1 s with nothing connected to the encoder | `stall` fault after about one second, both pins go low, log shows fault `1` |
| 7 | Send `s 100` | `# fault active, send clear first` |
| 8 | Send `clear`, then `stop` | `fault none`, pins stay low |
| 9 | Type `s abc` and `xyz` | `# unknown command, type help` both times |

## With the motor, open loop

Connect the DRV8871 and motor as in `hardware/wiring.svg`. Turn on the 12 V supply last.

| # | Step | Expected result |
|---|---|---|
| 10 | Send `open 30` | Motor turns slowly. `rpm` positive. If it is negative, see step 11 |
| 11 | If `rpm` was negative | After 1 s a `reversed` fault stops the motor. Swap encoder A and B, then `clear` |
| 12 | Send `open 100`, then `stop` | Close to the rated speed of the gear motor (about 300 rpm for the example). Coasts to rest after `stop` |
| 13 | Measure K and tau | See `tuning.md` |

## Closed loop

| # | Step | Expected result |
|---|---|---|
| 14 | Send `s 160` | Ramp up in about 0.3 s, then `rpm` within 2 rpm of 160. Duty settles somewhere around 50 to 60 percent |
| 15 | Send `s 240` | Rises with no visible overshoot, settles in under 0.5 s |
| 16 | Pinch the output shaft lightly with a cloth | `rpm` dips, duty rises, speed comes back to 240 within about half a second |
| 17 | Send `s -120` | Ramps down through zero and settles at -120 |
| 18 | Send `s 0` | Ramps to zero, then the duty goes to exactly 0 and the motor stays still without twitching |
| 19 | Send `s 200`, then hold the shaft firmly | Duty climbs to 100 percent, after 1 s a `stall` fault, motor coasts |
| 20 | Send `clear`, `s 200`, release | Normal start with no jump: the integrator was reset by the fault |
| 21 | Send `kp 0.008` and `s 240` | Overshoot and a harsher sound. Confirms the gains change live |
| 22 | Send `log off`, then `status` | Rows stop. Status line still prints |

## Supply and current checks

| # | Step | Expected result |
|---|---|---|
| 23 | Ammeter in the 12 V line, `s 240` with no load | Well under 1 A for a small gear motor |
| 24 | Stall the shaft for under a second with `open 100` | Current limited by the DRV8871 ILIM resistor (about 2 A with 30 kohm). Release quickly |
| 25 | Feel the DRV8871 after a few minutes at 240 rpm | Warm at most |

## PC side tests

`firmware/test/test_motor_ctl.c` holds expected results for every function in `motor_ctl.c`.
Build and run it with any PC C compiler:

```sh
cd firmware/test
cc -std=c11 -Wall -Wextra -I../App test_motor_ctl.c ../App/motor_ctl.c -o t && ./t
```

Expected output: `PASS`. The numbers in the test match `python tools/pi_sim.py --vectors`.

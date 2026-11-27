# Tuning on your motor

The default gains come from a model with a guessed motor. Every gear motor is a bit different, so
measure yours first. It takes about ten minutes.

## 1. Set the encoder resolution

Find the pulses per motor revolution (often 11 or 7) and the gear ratio on the motor label or
listing. Set `ENC_CPR` in `app.c` to `pulses x 4 x ratio` and rebuild.

Check it: send `open 40` and count the output shaft turns over 30 seconds with a stopwatch.
Turns x 2 should match the `rpm` column. If it reads double or half, the x4 or the ratio is wrong.
Send `stop` when done.

## 2. Measure K (speed per duty)

1. Send `open 60` and wait two seconds.
2. Read the `rpm` column. Call it `rpm60`.
3. Send `open 30` and read `rpm30`.
4. Send `stop`.

```
K          = (rpm60 - rpm30) / 0.30         rpm per unit duty
dead band  = 0.30 - rpm30 / K                duty where the motor starts to turn
```

Using two points instead of one removes most of the friction dead band from K.

## 3. Measure tau (how fast it spins up)

1. Send `stop`, wait for zero.
2. Send `open 60` and copy the next twenty CSV rows.
3. Find the row where `rpm` first reaches 63 percent of `rpm60`. The time since the command is tau.

The log runs at 10 Hz and the speed filter adds about 28 ms, so the answer is rough. For a better
number, temporarily set `LOG_EVERY` to 1 (100 rows per second) and `LPF_ALPHA` to 1.0.

## 4. Compute the gains

Pick the closed loop time constant `tau_c`. Start with `tau_c = tau`, which asks for the same
speed as the motor gives on its own. Smaller is faster but noisier.

```
Kp = tau / (K x tau_c)
Ki = Kp / tau
```

Example with K = 300 rpm, tau = 0.08 s, tau_c = 0.1 s: Kp = 0.00267, Ki = 0.0333.

Type them in with `kp 0.0027` and `ki 0.033`, then try steps with `s 100`, `s 200`, `s 50`.
When you are happy, put the values into `KP_DEFAULT` and `KI_DEFAULT`.

## 5. Try it in the simulator first

```sh
python tools/pi_sim.py --kp 0.0027 --ki 0.033 --tau 0.08 --cpr 1320
```

Edit `Motor(k_rpm=300.0 ...)` in `pi_sim.py` to your K. The summary shows rise time, overshoot,
steady error, the dip from a load step and how much the duty jitters. Results with the example
values:

| Gains | Overshoot | 2 percent settling | Load dip | Duty ripple |
|---|---|---|---|---|
| kp 0.0027, ki 0.033 (default) | 0.1 percent | 0.40 s | 19.2 rpm | 0.4 percent |
| kp 0.0054, ki 0.066 (double) | 3.4 percent | 0.36 s | 15.2 rpm | 0.9 percent |

Doubling both gains buys a little stiffness against load and costs overshoot and twice the duty
jitter, which you hear as a rougher motor.

## What wrong gains look like

| Symptom | Likely cause | Change |
|---|---|---|
| Slow to reach the target, no overshoot | Gains too low | Raise kp and ki together |
| Overshoot, then rings | ki too high for kp | Lower ki, or raise kp |
| Motor buzzes or hunts at steady speed | kp too high for the encoder resolution | Lower kp, or lower alpha a little |
| Never quite reaches the target | ki is 0 | Set ki |
| Runs at full speed, then `reversed` fault | Positive feedback | Swap encoder A/B or motor wires, then `clear` |

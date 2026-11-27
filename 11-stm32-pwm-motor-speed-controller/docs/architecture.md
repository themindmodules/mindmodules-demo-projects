# Architecture and timing

All numbers here are calculations from datasheet values or results from `tools/pi_sim.py`.
None of them are bench measurements.

## Signal flow

```
           TIM2 encoder counter (32 bit, x4)
                     |
 TIM10 100 Hz  ---> enc_delta ---> counts_to_rpm ---> lpf_step ---> rpm
 interrupt                                                           |
                 target (from "s 160") ---> ramp_step ---> setpoint  |
                                                             |       |
                                                         pi_step <---+
                                                             |
                                                         duty -1..+1
                                                             |
                                       fault_update ---> pwm_compute
                                                             |
                                     TIM3 CH1 (IN1), CH2 (IN2) at 20 kHz
                                                             |
                                                DRV8871 ---> motor
 main loop: commands from USART2, CSV row every 10th step
```

The whole control step runs inside the TIM10 interrupt, so its timing does not depend on how
long the serial output takes. The main loop only copies a snapshot of the state with interrupts
briefly disabled and prints it.

## PWM

| Item | Value |
|---|---|
| Timer clock | 84 MHz |
| ARR | 4199, so period = 4200 counts |
| Frequency | 84 MHz / 4200 = 20.0 kHz |
| Duty resolution | 1 / 4200 = 0.024 percent |

### Fast and slow decay

The DRV8871 has two inputs and no enable pin. Its truth table gives four states: forward
(IN1 high, IN2 low), reverse, coast (both low) and brake (both high). That allows two ways to
make a PWM signal.

| Mode | Forward | Off time | Behaviour |
|---|---|---|---|
| `DECAY_FAST` | IN1 = PWM, IN2 = 0 | coast | Current dies quickly. Speed is less linear with duty at light load |
| `DECAY_SLOW` (default) | IN1 = 1, IN2 = inverted PWM | brake | Current keeps flowing in the low side switches. Speed is close to linear with duty |

`pwm_compute` turns the signed duty into the two compare values. For slow decay forward at
50 percent with a period of 4200, IN1 is 4200 (always high) and IN2 is 2100. IN2 high means
brake, so the motor is driven for the other 2100 counts.

A duty of zero always gives both inputs low (coast). The DRV8871 inputs have internal pull-downs,
so the bridge also coasts while the STM32 is in reset or being flashed. Check your board's
datasheet for the sleep and wake time after both inputs have been low for a while: the first
pulse after a long stop may be delayed by around a millisecond, which the loop absorbs.

## Encoder and speed measurement

The example motor has 11 pulses per motor turn and a 30:1 gearbox. TIM2 counts every edge of
both channels, so:

```
CPR = 11 x 4 x 30 = 1320 counts per output revolution
```

| Item | Formula | Value |
|---|---|---|
| Counts per second at 300 rpm | 300 / 60 x 1320 | 6600 |
| Counts per 10 ms window at 300 rpm | 6600 x 0.01 | 66 |
| Speed of one count in one window | 60 / (1320 x 0.01) | 4.55 rpm |
| Time between edges at 300 rpm | 1 / 6600 | 152 us |
| TIM2 input filter 10 | fDTS / 16, 8 samples | 1.52 us, 1 percent of the edge gap |
| Time to wrap the 32 bit counter at 300 rpm | 2^32 / 6600 | 7.5 days |

`enc_delta` subtracts as unsigned and casts to signed, so the wrap is harmless anyway.

### Quantisation and the speed filter

One count per window is worth 4.55 rpm. At 240 rpm the true count per window is 52.8, so the raw
reading jumps between 52 and 53 counts (236.4 and 240.9 rpm). A first order filter smooths this:

```
rpm = rpm + alpha x (raw - rpm)
time constant = -dt / ln(1 - alpha) = 0.01 / 0.357 = 28 ms for alpha 0.30
```

Results from `pi_sim.py` with the default gains, steps of 160 and 240 rpm:

| alpha | Measured speed ripple at 240 rpm | Duty ripple | Overshoot on the 160 rpm step | 2 percent settling |
|---|---|---|---|---|
| 1.00 (no filter) | 4.5 rpm | 1.4 percent | 0.2 percent | 0.53 s |
| 0.30 (default) | 1.5 rpm | 0.4 percent | 0.1 percent | 0.40 s |
| 0.15 | 1.0 rpm | 0.3 percent | 9.5 percent | 0.53 s |

A heavier filter adds lag inside the loop and the overshoot grows quickly. 0.30 is the balance
point for this model. With a finer encoder (more CPR) you can raise alpha.

The other option for slow speeds is to measure the time between edges with input capture instead
of counting edges per window. That is not done here, so speeds below about 20 rpm are rough.

## PI controller

The motor is treated as a first order system from duty to speed:

```
G(s) = K / (tau s + 1)
K   = speed at full duty, about 300 rpm for the example motor (measure yours)
tau = mechanical time constant, assumed 0.08 s (measure yours)
```

The PI zero is placed on the motor pole (Ti = tau). The closed loop then behaves like a first order
system with the time constant you choose, tau_c:

```
Kp = tau / (K x tau_c) = 0.08 / (300 x 0.1) = 0.00267   ->  0.0027 duty per rpm
Ki = Kp / tau          = 0.00267 / 0.08    = 0.0333     ->  0.033 duty per rpm per s
rise time 10 to 90 percent = 2.2 x tau_c = 0.22 s
```

`pi_sim.py` gives a rise time of 0.22 s, 0.1 percent overshoot and a load dip of 19 rpm for a
sudden load worth 40 rpm, with the speed back within 1 rpm about half a second later. `docs/tuning.md` shows
how to measure K and tau on your motor and redo the sum.

### Anti-windup

The integrator stops moving whenever the output is saturated at plus or minus 100 percent, unless
the error has changed sign and would pull the output back. The integrator value is also clamped
to the output range. Without this, a long stall or a big step would charge the integrator and the
motor would overshoot badly when it frees up.

### Setpoint ramp and parking

The setpoint moves toward the target at 600 rpm per second (6 rpm per step). This avoids current
spikes and keeps the loop out of saturation on normal steps. When the ramped setpoint is zero and
the measured speed is under 2 rpm, the integrator is cleared and the bridge coasts. Without that,
quantisation noise would make the motor twitch at standstill.

## Fault monitor

| Fault | Condition, held for 1 s (100 steps) | Usual cause |
|---|---|---|
| `stall` | duty at least 50 percent and speed under 5 rpm | Jammed shaft, encoder unplugged, motor supply off |
| `reversed` | duty at least 30 percent, speed over 20 rpm the opposite way | Motor wires or encoder A/B swapped |

On a fault the duty goes to zero (coast), the mode goes to `off`, and speed commands are refused
until `clear` is sent. A reversed loop is positive feedback and would otherwise run the motor at
full speed. The DRV8871 also has its own current limit and thermal shutdown, so the firmware
fault is a second layer, not the only one.

## CPU and serial load

| Item | Estimate |
|---|---|
| One control step | a few hundred single precision FPU instructions, roughly 5 us at 84 MHz, about 0.05 percent CPU |
| CSV row | about 30 bytes, 2.6 ms of UART time at 115200, 10 rows per second = 2.6 percent of the link |
| Command line | up to 39 characters, one byte per interrupt |

`HAL_UART_Transmit` blocks the main loop while a row goes out. The control loop is not affected
because it runs in the timer interrupt.

## Shared data

The interrupt writes the speed, duty and mode. The main loop changes the target, gains and mode.
Every multi field change in the main loop is wrapped in `__disable_irq` / `__enable_irq`, which is
a few instructions long, so the controller never sees half of an update.

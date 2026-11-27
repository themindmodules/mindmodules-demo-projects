# STM32 PWM DC Motor Speed Controller

Closed loop speed control for a small 12 V DC gear motor with a hall encoder, on an STM32F411
(Nucleo-F411RE) and a DRV8871 H-bridge. TIM3 makes 20 kHz PWM, TIM2 reads the encoder in x4 mode,
a PI loop runs at 100 Hz, and you set the speed and tune the gains live over the USB serial port.

> **Status:** reference design. `firmware/App` compiles clean for Cortex-M4F with
> `-Wall -Wextra -Werror -Wshadow` (see `firmware/test/check_build.sh`). The loop was checked in
> `tools/pi_sim.py` against a first order motor model, but the motor values in that model are
> assumptions, not a measured motor. The expected values in `firmware/test/test_motor_ctl.c` match
> the Python copy of the same arithmetic, but the C test itself was **not run**, because no PC C
> compiler was available here. The firmware has **not** been run on hardware and no motor was
> connected. `docs/test-plan.md` is the bring-up checklist.

## What you get

| Path | Contents |
|---|---|
| `firmware/App/app.c`, `app.h` | HAL glue: 100 Hz control interrupt, PWM output, encoder read, serial commands, CSV log |
| `firmware/App/motor_ctl.c`, `motor_ctl.h` | Pure C, no HAL: encoder maths, speed filter, setpoint ramp, PI with anti-windup, H-bridge PWM mapping, fault monitor, command parser, number formatting |
| `firmware/test/test_motor_ctl.c` | Expected results for every function in `motor_ctl.c` |
| `firmware/test/check_build.sh` | Compile check with `arm-none-eabi-gcc` and a stub HAL header |
| `tools/pi_sim.py` | Step and load response simulation of the loop, plus the test vectors |
| `docs/cubemx-config.md` | Exact CubeMX settings: clocks, TIM3, TIM2, TIM10, USART2 |
| `docs/architecture.md` | Signal flow, PWM and decay modes, encoder resolution, filter, PI maths, faults, CPU load |
| `docs/tuning.md` | Measure your motor and work out the gains |
| `docs/test-plan.md` | Step by step bring-up and what pass looks like |
| `docs/bom.csv` | Parts for the bench |
| `hardware/wiring.svg` | Wiring diagram |

## Quick start

1. Create a CubeIDE or CubeMX project for the **NUCLEO-F411RE** and apply `docs/cubemx-config.md`.
2. Copy `firmware/App/*` into the project (for example `Core/Src` and `Core/Inc`).
3. In `main.c` add:

```c
/* USER CODE BEGIN Includes */
#include "app.h"
/* USER CODE END Includes */

/* USER CODE BEGIN 2 */
app_init(&htim3, &htim2, &htim10, &huart2);
/* USER CODE END 2 */

while (1) {
  /* USER CODE BEGIN 3 */
  app_poll();
  /* USER CODE END 3 */
}
```

4. Set `ENC_CPR` at the top of `app.c` for your motor (pulses x 4 x gear ratio).
5. Wire it as in `hardware/wiring.svg`. Open a terminal at 115200 8N1. Turn on the 12 V supply last.
6. Type commands and press Enter:

```
s 160          closed loop, 160 rpm forward
s -80          80 rpm reverse
open 40        40 percent duty, no feedback
stop           coast
kp 0.003       change a gain live
status
```

The log prints 10 rows per second:

```
ms,target_rpm,rpm,duty_pct,fault
5100,160.0,159.9,57.1,0
5200,160.0,160.0,57.1,0
```

## Design notes

- Duty is a signed number from -1 to +1 in the code. One function, `pwm_compute`, turns it into
  the two compare values for the bridge. Slow decay (brake in the off time) is the default
  because speed follows duty more linearly.
- The default gains come from placing the PI zero on the motor pole: `Kp = tau / (K x tau_c)`,
  `Ki = Kp / tau`. `docs/tuning.md` shows how to measure K and tau with the `open` command.
- One encoder count in a 10 ms window is 4.55 rpm for the example motor. A first order filter
  with alpha 0.30 brings the measured ripple from 4.5 to 1.5 rpm in the simulation without
  adding overshoot.
- The integrator freezes while the output is saturated, so a stall or a big step does not wind it up.
- If the motor stalls at high duty, or turns the opposite way to the command (swapped wires make
  the loop positive feedback), the firmware stops the bridge after one second and waits for `clear`.
- The DRV8871 has its own current limit and thermal shutdown. Its inputs are pulled low inside
  the chip, so the motor coasts while the STM32 is in reset.

## Safety

The motor runs from a 12 V DC adapter. Nothing here touches mains voltage. Keep fingers and loose
cables away from the output shaft during tests, and keep stall tests short: even with the current
limit, a stalled motor heats up quickly.

## Check the build

```sh
cd firmware/test
sh check_build.sh      # needs arm-none-eabi-gcc on PATH
```

## Run the simulation

```sh
python tools/pi_sim.py              # table and summary
python tools/pi_sim.py --kp 0.005   # try other gains
```

## License

Personal and educational use for MindModules Inner Circle subscribers. See `LICENSE.md`.

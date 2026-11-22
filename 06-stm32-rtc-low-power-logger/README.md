# STM32L432 RTC Wake-up Low-Power Logger

A logger that sleeps in Stop 2 and wakes on the RTC every 60 seconds. Each wake-up it measures its own
supply voltage (through VREFINT, no divider), the die temperature and one external sensor, stores a
12-byte record in RAM and goes back to sleep. Every 16 records it prints them over UART.
The planning budget is about 1.6 uA average at a 60 s period.

> **Status:** reference design. The application layer (`firmware/App`) compiles clean for
> Cortex-M4F with `-Wall -Wextra -Wconversion -Werror` against a HAL stub (see `firmware/test/check_build.sh`).
> The maths is mirrored in `tools/lp_model.py`, whose 25 self-test checks pass and which produced the
> tables in `docs/power-budget.md`. Nothing here has been run on a board yet. The current figures are
> datasheet-based estimates, not measurements. See `docs/test-plan.md` for the bring-up checklist.

## What you get

| Path | Contents |
|---|---|
| `firmware/App/` | `app.c/.h` (HAL glue, Stop 2 loop, RTC callback) and `lp_math.c/.h` (pure C, no HAL) |
| `firmware/test/` | Compile check script and a minimal HAL stub |
| `tools/lp_model.py` | PC model of the maths: self test plus current budget and battery life tables |
| `docs/cubemx-config.md` | CubeMX settings: LSE, RTC wake-up, ADC internal channels, pins, USART2 |
| `docs/architecture.md` | Wake-up cycle, why Stop 2, VREFINT and temperature maths, timing |
| `docs/power-budget.md` | Where the charge goes, battery life for three cell types, how to measure |
| `docs/test-plan.md` | Step-by-step bring-up and what "pass" looks like |
| `docs/bom.csv` | Parts for the bench |
| `hardware/wiring.svg` | Wiring diagram |

## Quick start

1. Create a CubeMX or CubeIDE project for the **NUCLEO-L432KC** (or any STM32L432).
2. Apply `docs/cubemx-config.md` and generate code. Tick "Set all free pins as analog".
3. Copy `firmware/App/*` into the project (for example `Core/Src` and `Core/Inc`).
4. In `main.c`:

```c
/* USER CODE BEGIN 2 */
app_init(&hadc1, &huart2, &hrtc, SystemClock_Config);
/* USER CODE END 2 */

while (1) {
  /* USER CODE BEGIN 3 */
  app_step();
  /* USER CODE END 3 */
}
```

5. Open the ST-LINK virtual COM port at 115200 8N1. About 15 minutes after reset you should see 16 lines like
   `t=60 vdda_mv=3297 temp_c=24.0 ext_mv=1650 fl=0` and a `wakes=16 drop=0 wut=15` summary (the first record is taken at boot, before any RTC wake-up).
   For a quicker first look set `APP_PERIOD_S` to 2 in `app.h`.

## Design notes

- **Measure the supply for free.** `VDDA = 3000 * VREFINT_CAL / VREFINT_raw`. A resistor divider would
  leak about as much current as the whole MCU asleep.
- **Power the sensor from a pin.** PA1 feeds the NTC divider only for about 1.5 ms per wake-up.
- **Sleep dominates.** At 60 s about 83 % of the charge is the Stop 2 current. Pin states and the
  debug setting matter more than code speed. See `docs/power-budget.md`.
- **Records survive sleep.** SRAM is kept in Stop 2, so the ring buffer needs no flash writes.

## Check it yourself

```sh
cd firmware/test
sh check_build.sh      # needs arm-none-eabi-gcc on PATH

cd ../../tools
python lp_model.py     # self test, then the budget tables
```

## License

Personal and educational use for MindModules Inner Circle subscribers. Do not redistribute the files. See `LICENSE.md`.

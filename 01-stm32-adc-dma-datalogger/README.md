# STM32G474 ADC + DMA Datalogger

4-channel, timer-triggered ADC sampling with DMA double-buffering on an STM32G474.
Each finished block is reduced to mean / min / max per channel and printed over UART.

> **Status:** reference design. The application layer (`firmware/App`) compiles clean for
> Cortex-M4 with `-Wall -Wextra -Werror` (see `firmware/test/check_build.sh`).
> It has **not** been run on hardware as part of this package. See `docs/test-plan.md`
> for the bring-up checklist.

## What you get

| Path | Contents |
|---|---|
| `firmware/App/` | `app.c/.h` (HAL glue, DMA callbacks) and `app_math.c/.h` (pure C, no HAL) |
| `docs/cubemx-config.md` | Exact CubeMX settings: clocks, ADC, DMA, TIM6, USART2 |
| `docs/architecture.md` | Data flow, timing and buffer maths |
| `docs/test-plan.md` | Step-by-step bring-up and what "pass" looks like |
| `docs/bom.csv` | Parts needed on the bench |
| `hardware/wiring.svg` | Wiring / block diagram |

## Quick start

1. Create a new CubeIDE / CubeMX project for **STM32G474** (any package with PA0-PA3 and a free USART).
2. Apply the settings in `docs/cubemx-config.md` and generate code.
3. Copy `firmware/App/*` into your project (for example `Core/Src` and `Core/Inc`).
4. In `main.c` add:

```c
/* USER CODE BEGIN Includes */
#include "app.h"
/* USER CODE END Includes */

/* USER CODE BEGIN 2 */
app_init(&hadc1, &huart2, &htim6);
/* USER CODE END 2 */

while (1) {
  /* USER CODE BEGIN 3 */
  app_poll();
  /* USER CODE END 3 */
}
```

5. Open a serial terminal at 115200 8N1. You should see one line per channel per block:
   `blk=12 ch=0 mean_mv=1650 min=2040 max=2056 ovr=0`

## Design notes

- Sample rate is set by **TIM6**, not by ADC free-run, so it is exact and easy to change.
- DMA runs in circular mode over two halves; callbacks only set a flag. All `snprintf`
  and UART work happens in `app_poll()`, so the ISR stays short.
- `ovr` counts blocks that were overwritten before the main loop consumed them. It must stay 0.

## Check the build

```sh
cd firmware/test
sh check_build.sh      # needs arm-none-eabi-gcc on PATH
```

## License

Personal and educational use for MindModules Inner Circle subscribers. Do not redistribute the files. See `LICENSE.md`.

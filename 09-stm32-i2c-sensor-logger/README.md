# STM32 I2C Sensor Scanner and Logger

Firmware for an STM32F411 (Nucleo-F411RE) that scans the I2C bus, prints an `i2cdetect` style table
with a guess of what each address is, then logs a temperature sensor (LM75 or TMP102) once per
second as timestamped CSV over the USB serial port.

> **Status:** reference design. `firmware/App` compiles clean for Cortex-M4 with
> `-Wall -Wextra -Werror` for both the LM75 and the TMP102 build (see `firmware/test/check_build.sh`).
> The expected values in `firmware/test/test_i2c_util.c` were checked against a small Python
> copy of the same arithmetic, but the C test itself was **not run**, because no PC C compiler
> was available here. The firmware has **not** been run on hardware as part of this package.
> `docs/test-plan.md` is the bring-up checklist.

## What you get

| Path | Contents |
|---|---|
| `firmware/App/app.c`, `app.h` | HAL glue: bus scan, register read, CSV logging, bus re-init after errors |
| `firmware/App/i2c_util.c`, `i2c_util.h` | Pure C, no HAL: address bitmap, device hints, LM75 and TMP102 conversion, number and CSV formatting |
| `firmware/test/test_i2c_util.c` | Expected results for every function in `i2c_util.c` |
| `firmware/test/check_build.sh` | Compile check with `arm-none-eabi-gcc` and a stub HAL header |
| `docs/cubemx-config.md` | Exact CubeMX settings: clock, I2C1, USART2 |
| `docs/architecture.md` | Data flow, timing, pull-up resistor maths, error handling |
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
app_init(&hi2c1, &huart2);
/* USER CODE END 2 */

while (1) {
  /* USER CODE BEGIN 3 */
  app_poll();
  /* USER CODE END 3 */
}
```

4. Wire the sensor as in `hardware/wiring.svg`, open a serial terminal at 115200 8N1.
5. You should see the scan table, then rows like:

```
ms,addr,temp_c,status
1003,0x48,23.500,0
2003,0x48,23.500,0
```

For a TMP102 add `APP_SENSOR_TMP102` to the compiler defines. The default is LM75.

## Design notes

- Addresses are 7 bit everywhere in the code. The shift left by one for HAL happens in one place.
- Temperatures are integers in milli-degrees, so no floating point or `%f` is needed.
- The log timer uses a signed time difference, so it keeps working when the millisecond counter
  wraps after 49.7 days.
- Three read errors in a row make the firmware re-initialise the I2C peripheral and print a
  `# i2c re-initialised` line. Status 1 in the CSV marks a failed read.

## Check the build

```sh
cd firmware/test
sh check_build.sh      # needs arm-none-eabi-gcc on PATH
```

## License

Personal and educational use for MindModules Inner Circle subscribers. See `LICENSE.md`.

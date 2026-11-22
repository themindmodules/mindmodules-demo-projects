# CubeMX configuration (STM32L432KC)

These are the settings the code in `firmware/App` expects. Anything not listed stays at the default.

## Clocks
| Item | Value |
|---|---|
| LSE | 32.768 kHz crystal, RTC clock source = LSE |
| SYSCLK | MSI 16 MHz (range 8), no PLL. Low clock keeps the awake current down |
| Voltage scaling | Range 2 is enough at 16 MHz and saves a little more |
| ADC clock | Synchronous HCLK / 1 (16 MHz) |

Keep `SystemClock_Config()` as generated. The firmware calls it again after each wake-up.

## RTC
| Setting | Value |
|---|---|
| Activate clock source | Yes |
| Calendar | Not used by this code, can stay off |
| WakeUp | **Internal WakeUp** |
| Wake-up clock / counter | Leave default. `app_init()` sets them with `HAL_RTCEx_SetWakeUpTimer_IT` |
| NVIC | **RTC wake-up interrupt through EXTI line 20** enabled |

## ADC1
| Setting | Value |
|---|---|
| IN5 | Single-ended (PA0) |
| Vrefint channel | Enabled |
| Temperature sensor channel | Enabled |
| Resolution | 12 bit, right aligned |
| Scan, continuous, DMA | All disabled. The code reads one channel at a time by polling |
| Low power auto wait | Disabled |
| Overrun | Data overwritten |

The code reconfigures rank 1 before each read, so the rank settings in CubeMX do not matter.

## GPIO
| Pin | Mode | Use |
|---|---|---|
| PA0 | Analog (ADC1_IN5) | NTC divider midpoint |
| PA1 | Output push-pull, low, low speed | Powers the NTC divider only while sampling |
| PB3 | Leave as is or set to analog | User LED on the Nucleo. The code never turns it on |
| All unused pins | **Analog** | Floating digital inputs leak current in Stop 2. Tick "Set all free pins as analog" in Project Manager > Code Generator |

## USART2
| Setting | Value |
|---|---|
| Mode | Asynchronous, 115200 8N1 |
| Pins | PA2 (TX) and PA15 (RX). These go to the ST-LINK virtual COM port on the NUCLEO-L432KC |

## Debug
Turn **off** "Debug in Stop mode" for current measurements. With a debugger attached and that option on,
the core clocks keep running and the current reading is meaningless.

## main.c
```c
/* USER CODE BEGIN Includes */
#include "app.h"
/* USER CODE END Includes */

/* USER CODE BEGIN 2 */
app_init(&hadc1, &huart2, &hrtc, SystemClock_Config);
/* USER CODE END 2 */

while (1) {
  /* USER CODE BEGIN 3 */
  app_step();
  /* USER CODE END 3 */
}
```

CubeMX already declares `void SystemClock_Config(void);` near the top of `main.c`, so it can be passed
straight to `app_init()`.

> Check the pin choices against the datasheet and the board user manual (UM1956) for your exact board
> revision. Confirm in CubeMX that PA0 is ADC1_IN5 on your package before wiring.

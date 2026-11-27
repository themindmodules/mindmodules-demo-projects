# CubeMX settings

Board: NUCLEO-F411RE (STM32F411RET6). Create the project from the board selector, accept the default
peripheral initialisation, then change the items below.

## Clocks

| Setting | Value |
|---|---|
| Clock source | HSI 16 MHz (no crystal needed) |
| PLL source | HSI |
| PLLM / PLLN / PLLP | 16 / 336 / 4 |
| SYSCLK | 84 MHz |
| AHB prescaler | 1 (HCLK 84 MHz) |
| APB1 prescaler | 2 (PCLK1 42 MHz, timer clock x2 = 84 MHz for TIM2 and TIM3) |
| APB2 prescaler | 1 (PCLK2 84 MHz, timer clock 84 MHz for TIM10) |

All three timers end up with an 84 MHz clock. `app.c` uses that number only to print the PWM
frequency in the banner.

## TIM3: PWM to the H-bridge

| Setting | Value |
|---|---|
| Channel 1 | PWM Generation CH1, pin PB4 (Arduino D5), goes to DRV8871 IN1 |
| Channel 2 | PWM Generation CH2, pin PB5 (Arduino D4), goes to DRV8871 IN2 |
| Prescaler | 0 |
| Counter mode | Up |
| Counter period (ARR) | 4199 |
| auto-reload preload | Enable |
| PWM mode | PWM mode 1, pulse 0, polarity High, output compare preload Enable |

84 MHz / 4200 = 20.0 kHz, above hearing, with 4200 duty steps. `app.c` reads ARR at start-up, so
if you change it the duty maths follows.

## TIM2: quadrature encoder

| Setting | Value |
|---|---|
| Combined channels | Encoder Mode |
| Pins | PA0 = TIM2_CH1 (Arduino A0), PA1 = TIM2_CH2 (Arduino A1) |
| Prescaler | 0 |
| Counter period | 4294967295 (0xFFFFFFFF, TIM2 is 32 bit) |
| Encoder mode | Encoder Mode TI1 and TI2 (x4 counting) |
| Input filter, both channels | 10 |
| Polarity, both channels | Rising edge |
| GPIO pull | Pull-up on PA0 and PA1 (most hall encoders on these motors are open collector) |

If the count goes down when the motor runs forward, swap the A and B wires or set one channel's
polarity to Falling. The firmware detects this case and stops with a `reversed` fault.

## TIM10: control loop tick

| Setting | Value |
|---|---|
| Activated | yes |
| Prescaler | 8399 (84 MHz / 8400 = 10 kHz) |
| Counter period | 99 (10 kHz / 100 = 100 Hz) |
| NVIC | TIM1 update and TIM10 global interrupt enabled, preemption priority 2 |

## USART2 (ST-LINK virtual COM port)

| Setting | Value |
|---|---|
| Mode | Asynchronous |
| Pins | PA2 = TX, PA3 = RX |
| Baud rate | 115200, 8 bits, no parity, 1 stop bit |
| NVIC | USART2 global interrupt enabled, preemption priority 3 |

The UART priority is lower than TIM10, so typing never delays the control loop.

## SysTick and the HAL time base

Keep SysTick as the HAL time base. If you switch the time base to a timer, CubeMX writes its own
`HAL_TIM_PeriodElapsedCallback` into `main.c`, which clashes with the one in `app.c`. In that case
delete the one in `app.c` and call its body from yours.

## After generating code

Copy `firmware/App/*` into `Core/Src` and `Core/Inc` and add the lines shown in the top level
README to `main.c`.

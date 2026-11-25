# CubeMX settings

Board: NUCLEO-F411RE (STM32F411RET6). Create the project from the board selector and accept the
default peripheral initialisation, then change the items below.

## Clocks

| Setting | Value |
|---|---|
| Clock source | HSI 16 MHz (no crystal needed) |
| PLL source | HSI |
| PLLM / PLLN / PLLP | 16 / 336 / 4 |
| SYSCLK | 84 MHz |
| AHB prescaler | 1 (HCLK 84 MHz) |
| APB1 prescaler | 2 (PCLK1 42 MHz, I2C1 clock) |
| APB2 prescaler | 1 |
| Flash latency | 2 wait states (set automatically) |

## I2C1

| Setting | Value |
|---|---|
| Mode | I2C |
| Pins | PB8 = I2C1_SCL (Arduino D15), PB9 = I2C1_SDA (Arduino D14) |
| I2C speed mode | Standard mode |
| I2C clock speed | 100000 Hz |
| Addressing mode | 7 bit |
| Clock no stretch | Disabled (stretching allowed) |
| GPIO pull | No pull-up and no pull-down, because the external 4.7 kohm resistors do the job |
| GPIO speed | Low or medium, open drain (CubeMX sets open drain for I2C) |

To try 400 kHz instead, choose Fast mode, 400000 Hz, duty cycle 2, and change the pull-ups to
2.2 kohm (see the pull-up maths in `architecture.md`).

## USART2 (ST-LINK virtual COM port)

| Setting | Value |
|---|---|
| Mode | Asynchronous |
| Pins | PA2 = TX, PA3 = RX |
| Baud rate | 115200 |
| Word length / parity / stop bits | 8 / none / 1 |

## SysTick

The default 1 ms SysTick from the HAL is used for `HAL_GetTick()`. Do not change its priority to
a level below the I2C interrupts if you later move to interrupt or DMA transfers.

## After generating code

Copy the files from `firmware/App/` into `Core/Src` and `Core/Inc` and add the three lines shown in
the top level README to `main.c`. Add `APP_SENSOR_TMP102` under Preprocessor Defined Symbols if
you use a TMP102.

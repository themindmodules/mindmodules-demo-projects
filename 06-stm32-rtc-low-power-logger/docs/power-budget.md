# Current budget and battery life

All numbers come from `tools/lp_model.py`, which uses the same integer formulas as
`lp_avg_current_na()` and `lp_life_hours()` in `firmware/App/lp_math.c`. Rerun it after changing any input.

```sh
cd tools
python lp_model.py
```

## Inputs (default scenario)

These are planning values taken from typical datasheet figures and simple timing estimates.
They are **not** measurements. Replace them with your own readings from the IDD jumper.

| Input | Value | Where it comes from |
|---|---|---|
| Stop 2 with RTC on LSE | 1.3 uA | STM32L432 datasheet typical range at 3 V and 25 C. Rises steeply with temperature |
| Awake current | 2.0 mA | MSI 16 MHz run current plus ADC, rounded up |
| Awake time per wake-up | 3 ms | See `architecture.md`. Generous on purpose |
| Sensor divider | 150 uA for 1.5 ms | 3.0 V across 20 kohm, `HAL_Delay(1)` plus conversions |
| UART flush | 2.0 mA for 74 ms | 844 characters at 115200 baud, CPU awake while sending |
| Flush every | 16 wake-ups | `APP_FLUSH_EVERY` |

The model does not subtract the Stop 2 current during the awake time, so it reads slightly high.

## Where the charge goes (period 60 s)

| Part | Charge per 60 s | Share |
|---|---|---|
| Stop 2 + RTC | 78.00 uC | 83.4 % |
| UART flush (1/16 of one) | 9.25 uC | 9.9 % |
| Awake, measuring | 6.00 uC | 6.4 % |
| Sensor divider on | 0.23 uC | 0.2 % |

At a one-minute period the sleep current is most of the bill. Making the code faster barely matters.
The things that move the result are the Stop 2 current itself (pin states, debug off, temperature)
and anything that leaks all the time.

## Average current and life

Usable capacity after derating: CR2032 225 mAh x 70 %, 2 x AAA 1000 mAh x 80 %, 2 x AA 2500 mAh x 80 %.

| Period | I avg | CR2032 | 2 x AAA | 2 x AA |
|---|---|---|---|---|
| 1 s | 16.8 uA | 1.1 years | 5.4 years | 13.6 years |
| 10 s | 2.85 uA | 6.3 years | 32 years | 80 years |
| 60 s | 1.56 uA | 11.5 years | 59 years | 146 years |
| 300 s | 1.35 uA | 13.3 years | 68 years | 169 years |
| 3600 s | 1.30 uA | 13.8 years | 70 years | 175 years |

## Read these numbers with care

- Anything past about 5 to 10 years is fiction. Coin cells lose about 1 % a year and alkaline cells
  2 to 3 % a year to self-discharge, which is about the same as this whole load. The table shows that
  the MCU is no longer the limit, not that the logger will run for a century.
- A CR2032 cannot deliver 2 mA pulses well when cold or near the end of life. Its voltage dips under
  load. Put 100 uF or more across it, and watch `vdda_mv` and the `fl` flag.
- Two fresh AA cells give about 3.2 V and fall to about 2.0 V. The STM32L432 runs down to 1.71 V, but
  check that your sensor still works at the bottom of that range.
- A NUCLEO board as a whole draws far more than this (ST-LINK, power LED, regulator). Only the current
  through the MCU IDD jumper can be compared with this table.

## How to measure

1. Remove the IDD jumper (JP1 on the NUCLEO-L432KC, check UM1956 for your board revision) and put a
   meter in its place on the uA range.
2. Build with "Debug in Stop mode" off and close the debug session.
3. Power-cycle the board so the debug logic is reset.
4. Most of the time the reading is the Stop 2 current. A meter will not show the 3 ms pulses well.
   For those use a current probe, a shunt and a scope, or a power profiler that logs at 10 kHz or more.
5. Put your measured values into `BASE` at the bottom of `tools/lp_model.py` and rerun it.

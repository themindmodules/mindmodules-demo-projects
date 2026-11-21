# Architecture and timing

```
TIM6 (10 kHz) --TRGO--> ADC1 scan (4 ch) --DMA--> s_buf[2 halves][256 scans][4 ch]
                                                   |  half / full callback
                                                   v
                                         s_ready flag (ISR)
                                                   |
                                         app_poll() in main loop
                                                   |
                                  mean / min / max -> snprintf -> USART2
```

## Numbers

| Quantity | Value |
|---|---|
| Scan rate (per channel) | 10 000 samples/s |
| Conversions per scan | 4 |
| ADC conversion time per channel | (47.5 + 12.5) cycles / 42.5 MHz = 1.41 us |
| Time for one 4-channel scan | about 5.6 us, far below the 100 us trigger period |
| Scans per half buffer | 256 |
| Half-buffer period | 256 / 10 kHz = 25.6 ms |
| Buffer size | 2 x 256 x 4 x 2 bytes = 4096 bytes |
| UART load | 4 lines of about 55 chars per block = about 220 chars per 25.6 ms = 8.6 kchar/s |

**UART bandwidth warning:** 115200 baud is about 11.5 kchar/s. 8.6 kchar/s fits, but with little
margin, and the blocking transmit in `app_poll()` takes about 19 ms of every 25.6 ms block.
If `ovr` rises, print every Nth block, raise the baud rate, or lower the scan rate.

## Why this structure

- **Timer trigger instead of continuous mode:** exact, adjustable sample rate.
- **Circular DMA with half/full callbacks:** the CPU never touches data being written.
- **Pure-C math in `app_math.c`:** no HAL dependency, so it can be unit-tested on a PC.
- **Integer maths only:** `mean_x10` keeps one decimal with no floating point.

## Limits of this demo

- Reference voltage is assumed to be 3300 mV. For real accuracy read VREFINT and use the factory calibration value.
- No anti-alias filter is specified. Add the RC low-pass from `docs/bom.csv` if the input has content above 5 kHz.

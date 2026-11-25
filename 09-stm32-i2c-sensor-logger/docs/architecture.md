# Architecture and timing

All numbers here are calculations from datasheet limits, not measurements.

## Data flow

```
 power up
    |
 app_init ---> print banner ---> app_scan_bus ---> pick first address in 0x48..0x4F
                                     |
                       HAL_I2C_IsDeviceReady for 0x08..0x77
                                     |
                       i2c_scan_t bitmap ---> table + hints printed
 main loop
    |
 app_poll ---> every 1000 ms ---> app_read_reg(addr, 0x00, 2 bytes)
                                     |
                       lm75_raw_to_mc or tmp102_raw_to_mc
                                     |
                       csv_line ---> HAL_UART_Transmit
```

Everything runs in the main loop. There are no interrupts or DMA in the application code, which
keeps the logic easy to follow. The HAL calls block while the bus is busy.

## Scan time

Each probe sends the address byte plus a start and stop. At 100 kHz one byte with its
acknowledge bit takes 9 clocks, or 90 microseconds.

| Case | Estimate |
|---|---|
| One address, no device answering, 2 trials | about 0.25 ms |
| Whole range 0x08 to 0x77 (112 addresses) | about 28 ms |
| Worst case if the bus is held low and every trial times out | 112 x 2 x 5 ms = 1.12 s |

Printing the table is about 450 bytes. At 115200 baud that is roughly 11.5 kbyte/s, so about 40 ms.

## CSV bandwidth

A row such as `123456,0x48,23.375,0` plus CRLF is under 30 bytes. That is about 2.5 ms of UART time
once per second, which is 0.25 percent of the link.

## Pull-up resistor maths

I2C lines are open drain, so the resistor sets the rise time. For standard mode the bus
specification allows a rise time (30 percent to 70 percent) of 1000 ns. The usual formula for the
rise time is `tr = 0.8473 x Rp x Cb`, where Cb is the total bus capacitance.

Limits for a 3.3 V bus, 3 mA sink current and 100 pF of bus capacitance:

| Limit | Formula | Value |
|---|---|---|
| Minimum Rp (sink current) | (3.3 V - 0.4 V) / 3 mA | 967 ohm |
| Maximum Rp, 100 kHz | 1000 ns / (0.8473 x 100 pF) | 11.8 kohm |
| Maximum Rp, 400 kHz (tr 300 ns) | 300 ns / (0.8473 x 100 pF) | 3.5 kohm |

| Chosen value | Bus capacitance | Rise time |
|---|---|---|
| 4.7 kohm | 100 pF | 398 ns (fine at 100 kHz) |
| 4.7 kohm | 200 pF | 796 ns (still inside 1000 ns) |
| 2.2 kohm | 100 pF | 186 ns (fine at 400 kHz) |

Most breakout boards already have their own pull-ups, often 10 kohm. They sit in parallel with your
resistors. With three such boards and 4.7 kohm on the Nucleo side the total is about 1.95 kohm,
which is still above the 967 ohm minimum.

## Sensor conversions

| Part | Register 0 format | Step | Example bytes | Result |
|---|---|---|---|---|
| LM75 | top 9 bits, two's complement | 0.5 C | 0x19 0x00 | 25.0 C |
| LM75 | | | 0xE7 0x00 | -25.0 C |
| TMP102 | top 12 bits, two's complement | 0.0625 C | 0x19 0x00 | 25.0 C |
| TMP102 | | | 0x7F 0xF0 | 127.937 C (127.9375 truncated) |
| TMP102 | | | 0xC9 0x00 | -55.0 C |

The unused low bits of these registers are zero in normal mode, so dividing by 128 or 16 acts
as an exact shift. A TMP102 in its 13 bit extended mode is not supported.

## Error handling

| Event | Behaviour |
|---|---|
| Read fails | CSV status field is 1 and the temperature column shows 0.000, so filter on status |
| 3 read fails in a row | `HAL_I2C_DeInit` then `HAL_I2C_Init`, prints `# i2c re-initialised` |
| No sensor at 0x48 to 0x4F | Scan still prints, logging stays off |
| Counter wraps (49.7 days) | Timer uses a signed difference, no skipped or doubled row |

A device that holds SDA low after a reset cannot be freed by re-initialising the peripheral
alone. The usual fix is nine clock pulses on SCL driven from GPIO. That is not implemented here.

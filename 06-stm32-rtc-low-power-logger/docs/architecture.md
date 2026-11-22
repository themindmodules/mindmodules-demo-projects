# Architecture and timing

## One wake-up cycle

```
          RTC wake-up timer (ck_spre, 1 Hz, reload = period - 1)
                              |
  Stop 2  ------------------> EXTI 20 interrupt, core wakes on MSI
                              |
                     restore_clocks()  (SystemClock_Config)
                              |
              PA1 high  ->  1 ms settle  ->  ADC: VREFINT, TEMP, IN5  ->  PA1 low
                              |
              lp_vdda_mv / lp_temp_c_x10 / lp_counts_to_mv  ->  12-byte record
                              |
                   lp_ring_push()  (32 records in SRAM)
                              |
                 every 16th wake-up: print all records on USART2
                              |
        HAL_SuspendTick()  ->  HAL_PWREx_EnterSTOP2Mode(WFI)  ->  back to the top
```

All state lives in normal SRAM. On the STM32L4, SRAM1 and SRAM2 are both kept in Stop 2,
so the ring buffer and counters survive every sleep. Only a reset or Standby would lose them.

## Why each choice

- **Stop 2, not Standby.** Standby is lower (well under 1 uA) but loses SRAM and restarts from reset,
  so every wake-up would pay for a full boot and the records would need backup registers or flash.
  Stop 2 keeps everything and wakes in a few microseconds.
- **RTC wake-up timer from ck_spre.** With the prescalers set for a 1 Hz calendar clock, the wake-up
  counter ticks once a second. The reload is `period - 1`. Periods above 65536 s use the 17-bit mode,
  which adds 2^16 to the count. `lp_wut_reload()` handles both and rejects 0 and anything over 131072 s.
- **Supply from VREFINT, no divider.** VREFINT is a fixed reference near 1.2 V. The factory stores its
  reading taken at VDDA = 3.000 V. So `VDDA = 3000 * VREFINT_CAL / VREFINT_raw`. This gives the battery
  voltage with zero standing current, where a 1 Mohm + 1 Mohm divider would draw 1.5 uA all the time,
  about as much as the whole MCU in Stop 2.
- **Temperature rescaled to VDDA.** The TS_CAL points were taken at 3.000 V. On a 2.7 V supply the same
  temperature gives a bigger raw count, so the reading is scaled by `VDDA / 3000` before the straight line
  through (TS_CAL1, 30 C) and (TS_CAL2, 130 C) is applied.
- **Sensor powered from a GPIO.** A 10k + 10k NTC divider draws 150 uA. Left on, that is 100 times the
  sleep current. PA1 powers it for about 1.5 ms per wake-up instead.
- **Batching the UART.** One 51-character line takes 4.4 ms at 115200 baud. Sending 16 lines at once
  keeps the printing out of the short measurement window and lets the terminal be unplugged most of the time.

## Timing numbers

| Quantity | Value |
|---|---|
| ADC clock | 16 MHz (HCLK / 1) |
| One conversion | (640.5 + 12.5) cycles / 16 MHz = 40.8 us |
| Minimum sampling time for the temp sensor | 5 us (datasheet), so 640.5 cycles is plenty |
| Three conversions | about 122 us plus start-up of the internal channels |
| Sensor settle | `HAL_Delay(1)` gives 1 to 2 ms. The RC at PA0 is 5 kohm x 10 nF = 50 us, so it is settled 20 times over |
| Awake time without UART | budgeted at 3 ms (clock restore, settle, ADC, maths) |
| Record size | 12 bytes, 32 records = 384 bytes of SRAM |
| Flush text | 16 lines x 51 chars + 28 char summary = 844 chars = 73 ms at 115200 |
| Ring headroom | 32 slots. `APP_FLUSH_EVERY` can go up to 32 before records are dropped |

## Output format

```
lp-logger period=60s flush=16
t=0 vdda_mv=3296 temp_c=24.1 ext_mv=1652 fl=0
t=60 vdda_mv=3297 temp_c=24.0 ext_mv=1650 fl=0
...
wakes=16 drop=0 wut=15
```

`fl=1` means VDDA was below 2200 mV for that record. `drop` counts records overwritten before a flush.
`wut` counts RTC wake-up interrupts and should always be `wakes - 1`, because the first record is taken at boot. The lines above show the format only.
They were not captured from a board.

## Limits of this demo

- `t` is wake count x period, not a calendar time. It drifts with the LSE (about 20 ppm, under 2 s a day).
  Read the RTC calendar if you need real time stamps.
- Records only leave the MCU over UART. A power cut loses up to 32 records. Add flash or FRAM storage
  for a logger that must keep its data.
- The internal sensor measures die temperature, not air temperature. Expect a few degrees of offset
  and a datasheet accuracy of about +/-3 C even after calibration.
- If an interrupt other than the RTC wakes the MCU early, `app_step()` still adds a full period to `t`.
  A gap between `wut` and `wakes - 1` shows that.

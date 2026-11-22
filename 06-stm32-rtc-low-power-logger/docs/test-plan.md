# Bring-up and test plan

None of these steps have been done on a board for this package. They are the checklist to follow.

| # | Step | Expected result |
|---|---|---|
| 1 | Set `APP_PERIOD_S` to 2 and `APP_FLUSH_EVERY` to 4, flash, open the terminal at 115200 | Header line, then 4 records every 8 s |
| 2 | Check `wakes` and `wut` in the summary line | `wut` is `wakes - 1`, and both go up by 4 each flush |
| 3 | Compare `vdda_mv` with a meter on the 3V3 pin | Within about 1 % |
| 4 | Read `temp_c` at room temperature, then warm the MCU with a finger | A few degrees above room, then rises |
| 5 | Remove the NTC divider. Tie PA0 to GND, then to 3V3 | `ext_mv` near 0, then near `vdda_mv` |
| 6 | Fit the NTC divider and warm the NTC | `ext_mv` changes. With the NTC on the high side it rises as the NTC warms |
| 7 | Scope PA1 | A pulse of 1 to 2 ms once per period, low the rest of the time |
| 8 | Period back to 60 s, debug in Stop off, meter on the IDD jumper | A few uA at most between wake-ups. Tens of uA means a pin or peripheral is still on |
| 9 | Set `APP_FLUSH_EVERY` to 40 (more than the 32 ring slots) and wait for a flush | `drop` reads 8. This proves the ring buffer overwrites the oldest data |
| 10 | Run from a CR2032 for a day (USB unplugged, 3.0 V into 3V3) | When USB is plugged back in the records keep coming, `fl=0` |

Step 9 is the only time `APP_FLUSH_EVERY` should be above `LP_RING_LEN`. Set it back afterwards.

## If the current is too high

- Unused pins left as digital inputs: set them all to analog.
- Debug in Stop mode left on, or the ST-LINK still connected to SWD.
- A peripheral clock left on that is not needed in Stop 2.
- The LED on PB3 driven high.
- Measuring the whole board instead of the IDD jumper.

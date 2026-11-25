# Bring-up and test plan

This plan has not been run on hardware. It lists what to expect from the code and calculations.

| # | Step | Expected result |
|---|---|---|
| 1 | Build with `APP_SENSOR_TMP102` undefined for an LM75, or defined for a TMP102. Flash, open the terminal at 115200 | Banner `I2C scanner and logger, sensor type ...` |
| 2 | Leave the I2C wires disconnected | `0 device(s) found` and `logging disabled`. Pull-ups only, no crash |
| 3 | Connect SDA to PB9, SCL to PB8, 3V3 and GND. Reset the board | The table shows one address, `0x48` for default pin strapping, with the hint `LM75, TMP102 or ADS1115` |
| 4 | Watch the CSV rows | One row per second. `ms` goes up by about 1000 |
| 5 | Hold a finger on the sensor for 30 seconds | `temp_c` rises by a few degrees |
| 6 | Compare with a reference thermometer at room temperature | Within the sensor datasheet accuracy, about 1 to 2 C for an LM75 |
| 7 | Disconnect SDA while logging | Status `1` rows, then `# i2c re-initialised` after the third failure |
| 8 | Reconnect SDA | Status returns to `0` within a few seconds. If not, power cycle the sensor, see the stuck bus note in `architecture.md` |
| 9 | Add a second I2C module (for example a BMP280 at 0x76) | Table shows both addresses and the matching hints. The log still uses 0x48 |
| 10 | Probe SCL with a scope at one address probe | 100 kHz clock. Rise time under 1000 ns |
| 11 | Switch to the TMP102 build and a TMP102 board | Resolution of 0.0625 C visible as 3 decimals (23.437, 23.500, ...) |

## PC side tests

`firmware/test/test_i2c_util.c` holds expected results for the conversion, formatting and scan
functions. Build and run it with any C compiler on the PC:

```sh
cc -std=c11 -Wall -Wextra firmware/test/test_i2c_util.c firmware/App/i2c_util.c -o t && ./t
```

Expected output: `PASS`.

If a row shows `0.000` with status 0, check that the sensor really answers register 0 as a
temperature register. Some parts at 0x48 to 0x4F (the ADS1115, for example) do not.

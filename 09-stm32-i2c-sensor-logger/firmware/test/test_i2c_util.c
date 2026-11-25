/* Expected values for the pure functions in i2c_util.c.
   Build this on the PC with any C compiler and run it, or add it to your own test setup.
   Prints PASS or each failing case, and returns non-zero on failure. */
#include <stdio.h>
#include <string.h>

#include "../App/i2c_util.h"

static int fails;

#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s\n", msg); fails++; } } while (0)

int main(void)
{
    const uint8_t lm_25[2] = {0x19, 0x00}, lm_m25[2] = {0xE7, 0x00}, lm_half[2] = {0x00, 0x80}, lm_mhalf[2] = {0xFF, 0x80};
    const uint8_t tm_25[2] = {0x19, 0x00}, tm_max[2] = {0x7F, 0xF0}, tm_m25[2] = {0xE7, 0x00}, tm_m55[2] = {0xC9, 0x00};
    char buf[32];
    i2c_scan_t s;

    CHECK(lm75_raw_to_mc(lm_25) == 25000, "lm75 +25.0 C");
    CHECK(lm75_raw_to_mc(lm_m25) == -25000, "lm75 -25.0 C");
    CHECK(lm75_raw_to_mc(lm_half) == 500, "lm75 +0.5 C");
    CHECK(lm75_raw_to_mc(lm_mhalf) == -500, "lm75 -0.5 C");
    CHECK(tmp102_raw_to_mc(tm_25) == 25000, "tmp102 +25.0 C");
    CHECK(tmp102_raw_to_mc(tm_max) == 127937, "tmp102 +127.9375 C truncated");
    CHECK(tmp102_raw_to_mc(tm_m25) == -25000, "tmp102 -25.0 C");
    CHECK(tmp102_raw_to_mc(tm_m55) == -55000, "tmp102 -55.0 C");

    CHECK(fmt_milli(buf, sizeof buf, 25000) == 6 && strcmp(buf, "25.000") == 0, "fmt 25.000");
    CHECK(fmt_milli(buf, sizeof buf, -500) == 6 && strcmp(buf, "-0.500") == 0, "fmt -0.500");
    CHECK(fmt_milli(buf, sizeof buf, 127937) == 7 && strcmp(buf, "127.937") == 0, "fmt 127.937");
    CHECK(fmt_milli(buf, 4, 25000) == 0, "fmt too small buffer");

    CHECK(csv_line(buf, sizeof buf, 1234, 0x48, 23375, 0) > 0 && strcmp(buf, "1234,0x48,23.375,0\r\n") == 0, "csv row");
    CHECK(csv_line(buf, 8, 1234, 0x48, 23375, 0) == 0, "csv row too small");

    i2c_scan_clear(&s);
    i2c_scan_mark(&s, 0x48);
    i2c_scan_mark(&s, 0x48);
    i2c_scan_mark(&s, 0x76);
    i2c_scan_mark(&s, 0x80);
    CHECK(s.count == 2, "scan count ignores duplicates and out of range");
    CHECK(i2c_scan_has(&s, 0x48) && i2c_scan_has(&s, 0x76) && !i2c_scan_has(&s, 0x49), "scan has");
    CHECK(i2c_scan_first_in(&s, 0x48, 0x4F) == 0x48, "first in range found");
    CHECK(i2c_scan_first_in(&s, 0x50, 0x57) == 0, "first in range empty");
    CHECK(strcmp(i2c_device_hint(0x76), "BMP280 or BME280") == 0, "hint 0x76");
    CHECK(strcmp(i2c_device_hint(0x7A), "unknown") == 0, "hint unknown");

    if (fails == 0) {
        printf("PASS\n");
    }
    return fails != 0;
}

#include "i2c_util.h"

#include <stdio.h>

void i2c_scan_clear(i2c_scan_t *s)
{
    for (unsigned i = 0; i < sizeof s->bits; i++) {
        s->bits[i] = 0;
    }
    s->count = 0;
}

void i2c_scan_mark(i2c_scan_t *s, uint8_t addr)
{
    if (addr > 0x7Fu || i2c_scan_has(s, addr)) {
        return;
    }
    s->bits[addr >> 3] |= (uint8_t)(1u << (addr & 7u));
    s->count++;
}

int i2c_scan_has(const i2c_scan_t *s, uint8_t addr)
{
    if (addr > 0x7Fu) {
        return 0;
    }
    return (s->bits[addr >> 3] >> (addr & 7u)) & 1u;
}

uint8_t i2c_scan_first_in(const i2c_scan_t *s, uint8_t lo, uint8_t hi)
{
    for (unsigned a = lo; a <= hi && a <= 0x7Fu; a++) {
        if (i2c_scan_has(s, (uint8_t)a)) {
            return (uint8_t)a;
        }
    }
    return 0;
}

const char *i2c_device_hint(uint8_t addr)
{
    switch (addr) {
    case 0x20: case 0x21: case 0x22: case 0x23:
        return "PCF8574 or MCP23017 expander (0x23 also BH1750)";
    case 0x27: case 0x3F:
        return "PCF8574 LCD backpack";
    case 0x29:
        return "VL53L0X or TSL2561";
    case 0x3C: case 0x3D:
        return "SSD1306 OLED";
    case 0x40:
        return "INA219, Si7021 or HTU21";
    case 0x44: case 0x45:
        return "SHT3x humidity sensor";
    case 0x48: case 0x49: case 0x4A: case 0x4B:
        return "LM75, TMP102 or ADS1115";
    case 0x50: case 0x51: case 0x52: case 0x53:
    case 0x54: case 0x55: case 0x56: case 0x57:
        return "24Cxx EEPROM";
    case 0x5A:
        return "MLX90614 or CCS811";
    case 0x68: case 0x69:
        return "DS3231 RTC or MPU6050 IMU";
    case 0x76: case 0x77:
        return "BMP280 or BME280";
    default:
        return "unknown";
    }
}

int32_t lm75_raw_to_mc(const uint8_t b[2])
{
    /* Top 9 bits of the 16 bit word, two's complement, 0.5 C per step. */
    int32_t raw = (int32_t)(int16_t)(uint16_t)(((uint16_t)b[0] << 8) | b[1]);
    raw /= 128;                          /* the 7 low bits are always zero on this part, so this is an exact shift */
    return raw * 500;
}

int32_t tmp102_raw_to_mc(const uint8_t b[2])
{
    /* Top 12 bits of the 16 bit word, two's complement, 0.0625 C per step. */
    int32_t raw = (int32_t)(int16_t)(uint16_t)(((uint16_t)b[0] << 8) | b[1]);
    raw /= 16;                           /* the 4 low bits are zero in 12 bit mode, so this is an exact shift */
    return (raw * 625) / 10;             /* 62.5 mC per step, truncated toward zero */
}

size_t fmt_milli(char *buf, size_t n, int32_t milli)
{
    unsigned neg = milli < 0;
    uint32_t a = neg ? (uint32_t)(-(int64_t)milli) : (uint32_t)milli;
    int len = snprintf(buf, n, "%s%lu.%03lu", neg ? "-" : "",
                       (unsigned long)(a / 1000u), (unsigned long)(a % 1000u));
    return (len > 0 && (size_t)len < n) ? (size_t)len : 0;
}

size_t csv_line(char *buf, size_t n, uint32_t ms, uint8_t addr, int32_t temp_mc, uint8_t status)
{
    char t[16];
    if (fmt_milli(t, sizeof t, temp_mc) == 0) {
        return 0;
    }
    int len = snprintf(buf, n, "%lu,0x%02X,%s,%u\r\n", (unsigned long)ms, (unsigned)addr, t, (unsigned)status);
    return (len > 0 && (size_t)len < n) ? (size_t)len : 0;
}

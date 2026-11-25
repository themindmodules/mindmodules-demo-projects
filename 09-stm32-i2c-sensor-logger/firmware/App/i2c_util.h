#ifndef I2C_UTIL_H
#define I2C_UTIL_H

#include <stddef.h>
#include <stdint.h>

#define I2C_ADDR_MIN 0x08u   /* 0x00-0x07 are reserved by the I2C spec */
#define I2C_ADDR_MAX 0x77u   /* 0x78-0x7F are reserved by the I2C spec */

/* Bitmap of 7-bit addresses that answered during a scan. */
typedef struct {
    uint8_t bits[16];        /* 128 addresses, one bit each */
    uint8_t count;           /* number of addresses marked  */
} i2c_scan_t;

void i2c_scan_clear(i2c_scan_t *s);
void i2c_scan_mark(i2c_scan_t *s, uint8_t addr);
int  i2c_scan_has(const i2c_scan_t *s, uint8_t addr);

/* First address inside [lo, hi] that answered, or 0 if none. */
uint8_t i2c_scan_first_in(const i2c_scan_t *s, uint8_t lo, uint8_t hi);

/* Short list of parts that commonly use this address. Only a hint, many parts share addresses. */
const char *i2c_device_hint(uint8_t addr);

/* Temperature conversions. Input is the two bytes read from register 0, MSB first.
   Output is milli-degrees Celsius. */
int32_t lm75_raw_to_mc(const uint8_t b[2]);     /* 9 bit, 0.5 C per step      */
int32_t tmp102_raw_to_mc(const uint8_t b[2]);   /* 12 bit, 0.0625 C per step  */

/* Writes a milli value as text with three decimals, for example 23375 -> "23.375". Returns length. */
size_t fmt_milli(char *buf, size_t n, int32_t milli);

/* One CSV row: ms,0xAA,temp_c,status followed by CRLF. Returns length, or 0 if it did not fit. */
size_t csv_line(char *buf, size_t n, uint32_t ms, uint8_t addr, int32_t temp_mc, uint8_t status);

#endif

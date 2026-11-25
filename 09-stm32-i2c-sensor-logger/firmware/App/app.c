#include "app.h"

#include <stdio.h>
#include <string.h>

#include "i2c_util.h"

#define LOG_ADDR_LO 0x48u        /* LM75, TMP102 and ADS1115 live in 0x48..0x4F */
#define LOG_ADDR_HI 0x4Fu
#define REG_TEMP    0x00u

static I2C_HandleTypeDef  *s_i2c;
static UART_HandleTypeDef *s_uart;
static i2c_scan_t          s_scan;
static uint8_t             s_log_addr;      /* 0 = nothing to log */
static uint32_t            s_next_ms;
static uint8_t             s_err_run;

static void say(const char *text)
{
    (void)HAL_UART_Transmit(s_uart, (uint8_t *)text, (uint16_t)strlen(text), 100u);
}

HAL_StatusTypeDef app_read_reg(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    /* HAL wants the 8 bit address: the 7 bit address shifted left by one. */
    return HAL_I2C_Mem_Read(s_i2c, (uint16_t)(addr << 1), reg, I2C_MEMADD_SIZE_8BIT, buf, len, 20u);
}

void app_scan_bus(void)
{
    char line[96];

    i2c_scan_clear(&s_scan);
    for (unsigned addr = I2C_ADDR_MIN; addr <= I2C_ADDR_MAX; addr++) {
        if (HAL_I2C_IsDeviceReady(s_i2c, (uint16_t)(addr << 1), APP_SCAN_TRIALS, APP_SCAN_TIMEOUT_MS) == HAL_OK) {
            i2c_scan_mark(&s_scan, (uint8_t)addr);
        }
    }

    say("     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\r\n");
    for (unsigned row = 0; row < 8; row++) {
        int n = snprintf(line, sizeof line, "%02X:", row * 16u);
        for (unsigned col = 0; col < 16; col++) {
            unsigned addr = row * 16u + col;
            if (addr < I2C_ADDR_MIN || addr > I2C_ADDR_MAX) {
                n += snprintf(line + n, sizeof line - (size_t)n, "   ");
            } else if (i2c_scan_has(&s_scan, (uint8_t)addr)) {
                n += snprintf(line + n, sizeof line - (size_t)n, " %02X", addr);
            } else {
                n += snprintf(line + n, sizeof line - (size_t)n, " --");
            }
        }
        snprintf(line + n, sizeof line - (size_t)n, "\r\n");
        say(line);
    }

    for (unsigned addr = I2C_ADDR_MIN; addr <= I2C_ADDR_MAX; addr++) {
        if (i2c_scan_has(&s_scan, (uint8_t)addr)) {
            snprintf(line, sizeof line, "found 0x%02X: %s\r\n", addr, i2c_device_hint((uint8_t)addr));
            say(line);
        }
    }
    snprintf(line, sizeof line, "%u device(s) found\r\n", (unsigned)s_scan.count);
    say(line);

    s_log_addr = i2c_scan_first_in(&s_scan, LOG_ADDR_LO, LOG_ADDR_HI);
}

void app_init(I2C_HandleTypeDef *hi2c, UART_HandleTypeDef *huart)
{
    s_i2c = hi2c;
    s_uart = huart;
    s_err_run = 0;

    say("\r\nI2C scanner and logger, sensor type " APP_SENSOR_NAME "\r\n");
    app_scan_bus();
    if (s_log_addr != 0) {
        say("ms,addr,temp_c,status\r\n");
    } else {
        say("no sensor in 0x48..0x4F, logging disabled\r\n");
    }
    s_next_ms = HAL_GetTick() + APP_LOG_PERIOD_MS;
}

static void log_row(uint32_t now)
{
    uint8_t raw[2] = {0, 0};
    char row[48];
    int32_t temp_mc = 0;
    uint8_t status = 0;

    if (app_read_reg(s_log_addr, REG_TEMP, raw, 2u) == HAL_OK) {
        s_err_run = 0;
#ifdef APP_SENSOR_TMP102
        temp_mc = tmp102_raw_to_mc(raw);
#else
        temp_mc = lm75_raw_to_mc(raw);
#endif
    } else {
        status = 1;
        if (++s_err_run >= APP_ERR_REINIT_AFTER) {
            /* A stuck or glitched bus: reset the peripheral and try again next period. */
            (void)HAL_I2C_DeInit(s_i2c);
            (void)HAL_I2C_Init(s_i2c);
            s_err_run = 0;
            say("# i2c re-initialised\r\n");
        }
    }

    if (csv_line(row, sizeof row, now, s_log_addr, temp_mc, status) != 0) {
        say(row);
    }
}

void app_poll(void)
{
    uint32_t now = HAL_GetTick();

    if (s_log_addr == 0) {
        return;
    }
    /* Signed difference keeps this correct when the millisecond counter wraps after 49.7 days. */
    if ((int32_t)(now - s_next_ms) >= 0) {
        s_next_ms += APP_LOG_PERIOD_MS;
        log_row(now);
    }
}

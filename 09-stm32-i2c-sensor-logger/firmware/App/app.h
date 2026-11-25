#ifndef APP_H
#define APP_H

#include "stm32f4xx_hal.h"

/* Pick the part you are logging. LM75 is 9 bit, TMP102 is 12 bit. Define APP_SENSOR_TMP102 to switch. */
#ifdef APP_SENSOR_TMP102
#define APP_SENSOR_NAME "TMP102"
#else
#define APP_SENSOR_NAME "LM75"
#endif

#define APP_LOG_PERIOD_MS   1000u   /* one CSV row per second                                   */
#define APP_SCAN_TRIALS        2u   /* HAL_I2C_IsDeviceReady attempts per address               */
#define APP_SCAN_TIMEOUT_MS    5u   /* timeout per attempt                                      */
#define APP_ERR_REINIT_AFTER   3u   /* consecutive read errors before the bus is re-initialised */

/* Call once after MX_I2C1_Init and MX_USART2_UART_Init. Prints a banner and scans the bus. */
void app_init(I2C_HandleTypeDef *hi2c, UART_HandleTypeDef *huart);

/* Call from the main while(1) loop. Logs one CSV row every APP_LOG_PERIOD_MS. */
void app_poll(void);

/* Scan 0x08..0x77 and print an i2cdetect style table. Can be called again at any time. */
void app_scan_bus(void);

/* Read len bytes starting at register reg of the device at 7-bit address addr. */
HAL_StatusTypeDef app_read_reg(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len);

#endif

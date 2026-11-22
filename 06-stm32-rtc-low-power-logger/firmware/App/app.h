#ifndef APP_H
#define APP_H

#include "stm32l4xx_hal.h"

#define APP_PERIOD_S       60u    /* seconds between wake-ups                  */
#define APP_FLUSH_EVERY    16u    /* wake-ups per UART flush (<= LP_RING_LEN)  */
#define APP_VDDA_LOW_MV    2200u  /* below this a record gets LP_FLAG_VDDA_LOW */
#define APP_SENSOR_SETTLE_MS 1u   /* sensor power on to sample                 */

/* Sensor supply pin (powers the NTC divider only while sampling). */
#define APP_SENSOR_PORT    GPIOA
#define APP_SENSOR_PIN     GPIO_PIN_1
/* External input on PA0 = ADC1_IN5 on the STM32L432. */
#define APP_EXT_CHANNEL    ADC_CHANNEL_5

/*
 * Call once after the CubeMX init functions.
 * restore_clocks is normally SystemClock_Config from main.c. It is called after
 * every wake-up because the MCU leaves Stop 2 on MSI at the MSISRANGE value (4 MHz by default).
 */
void app_init(ADC_HandleTypeDef *hadc, UART_HandleTypeDef *huart,
              RTC_HandleTypeDef *hrtc, void (*restore_clocks)(void));

/* One full cycle: measure, store, maybe flush, then sleep in Stop 2 until the
   next RTC wake-up. Call it from the main while(1) loop. */
void app_step(void);

#endif

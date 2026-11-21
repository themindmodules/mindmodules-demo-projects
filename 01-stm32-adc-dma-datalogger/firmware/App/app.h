#ifndef APP_H
#define APP_H

#include "stm32g4xx_hal.h"

#define APP_N_CH     4u      /* ADC1 channels per scan (IN1..IN4 = PA0..PA3) */
#define APP_N_SCANS  256u    /* scans per DMA half-buffer                     */
#define APP_VREF_MV  3300u   /* analog supply; use VREFINT cal for accuracy   */

/* Call once after MX_ADC1_Init / MX_DMA_Init / MX_TIM6_Init / MX_USART2_UART_Init */
void app_init(ADC_HandleTypeDef *hadc, UART_HandleTypeDef *huart, TIM_HandleTypeDef *htim);

/* Call from the main while(1) loop; prints one line per finished block. */
void app_poll(void);

#endif

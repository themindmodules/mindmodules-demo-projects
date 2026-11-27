/* HAL glue for the motor speed controller. */
#ifndef APP_H
#define APP_H

#include "stm32f4xx_hal.h"

/* pwm:  TIM3, CH1 = IN1 (PB4), CH2 = IN2 (PB5), 20 kHz
   enc:  TIM2 in encoder mode TI1 and TI2 (PA0, PA1), 32 bit counter
   loop: TIM10 update interrupt at 100 Hz
   uart: USART2, 115200 8N1 (ST-LINK virtual COM port) */
void app_init(TIM_HandleTypeDef *pwm, TIM_HandleTypeDef *enc,
              TIM_HandleTypeDef *loop, UART_HandleTypeDef *uart);

/* Call from the main loop. Handles commands and the CSV log. */
void app_poll(void);

#endif

/* Minimal stub so app.c can be syntax-checked without the Cube HAL installed.
   NOT for building real firmware - use the real STM32CubeG4 headers there. */
#ifndef STUB_HAL_H
#define STUB_HAL_H
#include <stdint.h>
typedef struct { int x; } ADC_HandleTypeDef;
typedef struct { int x; } UART_HandleTypeDef;
typedef struct { int x; } TIM_HandleTypeDef;
typedef enum { HAL_OK = 0 } HAL_StatusTypeDef;
#define ADC_SINGLE_ENDED 0u
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef *, uint32_t);
HAL_StatusTypeDef HAL_ADC_Start_DMA(ADC_HandleTypeDef *, uint32_t *, uint32_t);
HAL_StatusTypeDef HAL_TIM_Base_Start(TIM_HandleTypeDef *);
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *, uint8_t *, uint16_t, uint32_t);
#endif

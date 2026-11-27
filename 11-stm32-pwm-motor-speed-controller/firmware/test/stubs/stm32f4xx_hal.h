/* Minimal stub so app.c can be compile-checked without the Cube HAL installed.
   NOT for building real firmware. Use the real STM32CubeF4 headers there. */
#ifndef STUB_HAL_H
#define STUB_HAL_H
#include <stdint.h>

typedef struct {
    volatile uint32_t CNT;
    volatile uint32_t ARR;
    volatile uint32_t CCR1;
    volatile uint32_t CCR2;
} TIM_TypeDef;
typedef struct { TIM_TypeDef *Instance; } TIM_HandleTypeDef;
typedef struct { int x; } UART_HandleTypeDef;
typedef enum { HAL_OK = 0, HAL_ERROR = 1 } HAL_StatusTypeDef;

#define TIM_CHANNEL_1   0x00000000u
#define TIM_CHANNEL_2   0x00000004u
#define TIM_CHANNEL_ALL 0x0000003Cu

#define __HAL_TIM_GET_COUNTER(h)      ((h)->Instance->CNT)
#define __HAL_TIM_GET_AUTORELOAD(h)   ((h)->Instance->ARR)
#define __HAL_TIM_SET_COMPARE(h, ch, v) \
    ((ch) == TIM_CHANNEL_1 ? ((h)->Instance->CCR1 = (v)) : ((h)->Instance->CCR2 = (v)))

static inline void __disable_irq(void) {}
static inline void __enable_irq(void) {}

HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *, uint32_t);
HAL_StatusTypeDef HAL_TIM_Encoder_Start(TIM_HandleTypeDef *, uint32_t);
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *);
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *, uint8_t *, uint16_t, uint32_t);
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *, uint8_t *, uint16_t);
uint32_t HAL_GetTick(void);
#endif

/* Minimal stub so app.c can be compile-checked without the Cube HAL installed.
   NOT for building real firmware. Use the real STM32CubeL4 headers there. */
#ifndef STUB_HAL_H
#define STUB_HAL_H
#include <stdint.h>
typedef struct { int x; } ADC_HandleTypeDef;
typedef struct { int x; } UART_HandleTypeDef;
typedef struct { int x; } RTC_HandleTypeDef;
typedef struct { int x; } GPIO_TypeDef;
typedef enum { HAL_OK = 0, HAL_ERROR = 1 } HAL_StatusTypeDef;
typedef enum { GPIO_PIN_RESET = 0, GPIO_PIN_SET = 1 } GPIO_PinState;
typedef struct {
    uint32_t Channel, Rank, SamplingTime, SingleDiff, OffsetNumber, Offset;
} ADC_ChannelConfTypeDef;
extern GPIO_TypeDef stub_gpioa;
#define GPIOA (&stub_gpioa)
#define GPIO_PIN_1 0x0002u
#define ADC_CHANNEL_5 5u
#define ADC_CHANNEL_VREFINT 0x100u
#define ADC_CHANNEL_TEMPSENSOR 0x200u
#define ADC_REGULAR_RANK_1 1u
#define ADC_SAMPLETIME_640CYCLES_5 7u
#define ADC_SINGLE_ENDED 0u
#define ADC_OFFSET_NONE 4u
#define RTC_WAKEUPCLOCK_CK_SPRE_16BITS 4u
#define RTC_WAKEUPCLOCK_CK_SPRE_17BITS 6u
#define PWR_STOPENTRY_WFI 1u
/* Factory calibration addresses and constants as named in stm32l4xx_ll_adc.h */
#define VREFINT_CAL_ADDR ((uint16_t *)0x1FFF75AAu)
#define VREFINT_CAL_VREF 3000u
#define TEMPSENSOR_CAL1_ADDR ((uint16_t *)0x1FFF75A8u)
#define TEMPSENSOR_CAL2_ADDR ((uint16_t *)0x1FFF75CAu)
#define TEMPSENSOR_CAL1_TEMP 30
#define TEMPSENSOR_CAL2_TEMP 130
#define TEMPSENSOR_CAL_VREFANALOG 3000u
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef *, uint32_t);
HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef *, ADC_ChannelConfTypeDef *);
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *);
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *);
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *, uint32_t);
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *);
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *, uint8_t *, uint16_t, uint32_t);
HAL_StatusTypeDef HAL_RTCEx_SetWakeUpTimer_IT(RTC_HandleTypeDef *, uint32_t, uint32_t);
void HAL_GPIO_WritePin(GPIO_TypeDef *, uint16_t, GPIO_PinState);
void HAL_Delay(uint32_t);
void HAL_SuspendTick(void);
void HAL_ResumeTick(void);
void HAL_PWREx_EnterSTOP2Mode(uint8_t);
#endif

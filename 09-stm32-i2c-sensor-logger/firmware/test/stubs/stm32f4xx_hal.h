/* Minimal stub so app.c can be syntax-checked without the Cube HAL installed.
   NOT for building real firmware - use the real STM32CubeF4 headers there. */
#ifndef STUB_HAL_H
#define STUB_HAL_H
#include <stdint.h>
typedef struct { int x; } I2C_HandleTypeDef;
typedef struct { int x; } UART_HandleTypeDef;
typedef enum { HAL_OK = 0, HAL_ERROR = 1 } HAL_StatusTypeDef;
#define I2C_MEMADD_SIZE_8BIT 1u
HAL_StatusTypeDef HAL_I2C_IsDeviceReady(I2C_HandleTypeDef *, uint16_t, uint32_t, uint32_t);
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *, uint16_t, uint16_t, uint16_t, uint8_t *, uint16_t, uint32_t);
HAL_StatusTypeDef HAL_I2C_DeInit(I2C_HandleTypeDef *);
HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef *);
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *, uint8_t *, uint16_t, uint32_t);
uint32_t HAL_GetTick(void);
#endif

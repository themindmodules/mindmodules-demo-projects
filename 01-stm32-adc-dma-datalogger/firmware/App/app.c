/*
 * STM32G474 ADC + DMA datalogger (application layer)
 *
 * TIM6 TRGO -> ADC1 (4-channel scan) -> DMA circular buffer (2 halves).
 * The DMA half/full callbacks only mark a half as ready; all formatting and
 * UART output happens in app_poll() from the main loop, never in the ISR.
 */
#include "app.h"
#include "app_math.h"
#include <stdio.h>

static ADC_HandleTypeDef  *s_adc;
static UART_HandleTypeDef *s_uart;
static TIM_HandleTypeDef  *s_tim;

static uint16_t s_buf[2u * APP_N_SCANS * APP_N_CH];
static volatile int8_t s_ready = -1;   /* -1 none, 0 first half, 1 second half */
static volatile uint32_t s_overrun;
static uint32_t s_block_no;

void app_init(ADC_HandleTypeDef *hadc, UART_HandleTypeDef *huart, TIM_HandleTypeDef *htim)
{
    s_adc = hadc;
    s_uart = huart;
    s_tim = htim;

    /* Offset/linearity calibration before the first conversion. */
    (void)HAL_ADCEx_Calibration_Start(s_adc, ADC_SINGLE_ENDED);
    (void)HAL_ADC_Start_DMA(s_adc, (uint32_t *)s_buf, 2u * APP_N_SCANS * APP_N_CH);
    (void)HAL_TIM_Base_Start(s_tim);
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc != s_adc) return;
    if (s_ready != -1) s_overrun++;
    s_ready = 0;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc != s_adc) return;
    if (s_ready != -1) s_overrun++;
    s_ready = 1;
}

void app_poll(void)
{
    int8_t half = s_ready;
    if (half < 0) return;

    const uint16_t *blk = &s_buf[(uint32_t)half * APP_N_SCANS * APP_N_CH];
    char line[96];

    for (uint32_t ch = 0; ch < APP_N_CH; ch++) {
        ch_stats_t st = app_channel_stats(blk, APP_N_SCANS, APP_N_CH, ch);
        uint32_t mean_mv = app_counts_to_mv((st.mean_x10 + 5u) / 10u, APP_VREF_MV);
        int n = snprintf(line, sizeof line,
                         "blk=%lu ch=%lu mean_mv=%lu min=%u max=%u ovr=%lu\r\n",
                         (unsigned long)s_block_no, (unsigned long)ch,
                         (unsigned long)mean_mv, st.min, st.max,
                         (unsigned long)s_overrun);
        if (n > 0) {
            (void)HAL_UART_Transmit(s_uart, (uint8_t *)line, (uint16_t)n, 50u);
        }
    }
    s_block_no++;
    s_ready = -1;
}

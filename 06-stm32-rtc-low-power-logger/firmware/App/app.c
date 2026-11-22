/*
 * STM32L432 RTC wake-up low-power logger (application layer)
 *
 * The MCU spends almost all its time in Stop 2. The RTC wake-up timer brings it
 * back every APP_PERIOD_S seconds. Each wake-up it powers the sensor from a GPIO,
 * reads VREFINT, the internal temperature sensor and PA0, stores a 12-byte record
 * in a RAM ring buffer and goes back to sleep. The UART is only used once every
 * APP_FLUSH_EVERY wake-ups, because sending text costs more charge than measuring.
 */
#include "app.h"
#include "lp_math.h"
#include <stdio.h>

static ADC_HandleTypeDef  *s_adc;
static UART_HandleTypeDef *s_uart;
static RTC_HandleTypeDef  *s_rtc;
static void (*s_restore_clocks)(void);

static lp_ring_t s_ring;
static lp_ts_cal_t s_ts_cal;
static uint16_t s_vrefint_cal;
static uint32_t s_uptime_s;
static uint32_t s_wakes;
static volatile uint32_t s_wut_events;

static void uart_print(const char *buf, int n)
{
    if (n > 0) {
        (void)HAL_UART_Transmit(s_uart, (uint8_t *)buf, (uint16_t)n, 100u);
    }
}

static uint16_t adc_read(uint32_t channel)
{
    ADC_ChannelConfTypeDef c = {0};
    c.Channel = channel;
    c.Rank = ADC_REGULAR_RANK_1;
    c.SamplingTime = ADC_SAMPLETIME_640CYCLES_5;  /* 40 us at 16 MHz, meets the 5 us sensor minimum */
    c.SingleDiff = ADC_SINGLE_ENDED;
    c.OffsetNumber = ADC_OFFSET_NONE;
    c.Offset = 0u;
    if (HAL_ADC_ConfigChannel(s_adc, &c) != HAL_OK) return 0u;

    uint16_t v = 0u;
    if (HAL_ADC_Start(s_adc) == HAL_OK &&
        HAL_ADC_PollForConversion(s_adc, 2u) == HAL_OK) {
        v = (uint16_t)HAL_ADC_GetValue(s_adc);
    }
    (void)HAL_ADC_Stop(s_adc);
    return v;
}

static void measure(lp_record_t *rec)
{
    HAL_GPIO_WritePin(APP_SENSOR_PORT, APP_SENSOR_PIN, GPIO_PIN_SET);
    HAL_Delay(APP_SENSOR_SETTLE_MS);

    uint16_t vref_raw = adc_read(ADC_CHANNEL_VREFINT);
    uint16_t ts_raw   = adc_read(ADC_CHANNEL_TEMPSENSOR);
    uint16_t ext_raw  = adc_read(APP_EXT_CHANNEL);

    HAL_GPIO_WritePin(APP_SENSOR_PORT, APP_SENSOR_PIN, GPIO_PIN_RESET);

    uint32_t vdda = lp_vdda_mv(vref_raw, s_vrefint_cal, VREFINT_CAL_VREF);
    rec->t_s = s_uptime_s;
    rec->vdda_mv = (uint16_t)vdda;
    rec->temp_c_x10 = (int16_t)lp_temp_c_x10(ts_raw, vdda, &s_ts_cal);
    rec->ext_mv = (uint16_t)lp_counts_to_mv(ext_raw, vdda);
    rec->flags = (vdda < APP_VDDA_LOW_MV) ? LP_FLAG_VDDA_LOW : 0u;
}

static void flush(void)
{
    char line[80];
    lp_record_t r;
    while (lp_ring_pop(&s_ring, &r)) {
        int32_t t = r.temp_c_x10;
        const char *sign = (t < 0) ? "-" : "";
        if (t < 0) t = -t;
        int n = snprintf(line, sizeof line,
                         "t=%lu vdda_mv=%u temp_c=%s%ld.%ld ext_mv=%u fl=%u\r\n",
                         (unsigned long)r.t_s, r.vdda_mv, sign,
                         (long)(t / 10), (long)(t % 10), r.ext_mv, r.flags);
        uart_print(line, n);
    }
    int n = snprintf(line, sizeof line, "wakes=%lu drop=%lu wut=%lu\r\n",
                     (unsigned long)s_wakes, (unsigned long)s_ring.dropped,
                     (unsigned long)s_wut_events);
    uart_print(line, n);
}

void app_init(ADC_HandleTypeDef *hadc, UART_HandleTypeDef *huart,
              RTC_HandleTypeDef *hrtc, void (*restore_clocks)(void))
{
    s_adc = hadc;
    s_uart = huart;
    s_rtc = hrtc;
    s_restore_clocks = restore_clocks;
    lp_ring_init(&s_ring);

    /* Factory calibration values from system memory. */
    s_vrefint_cal   = *VREFINT_CAL_ADDR;
    s_ts_cal.cal1   = *TEMPSENSOR_CAL1_ADDR;
    s_ts_cal.cal2   = *TEMPSENSOR_CAL2_ADDR;
    s_ts_cal.t1_c   = TEMPSENSOR_CAL1_TEMP;
    s_ts_cal.t2_c   = TEMPSENSOR_CAL2_TEMP;
    s_ts_cal.cal_vref_mv = TEMPSENSOR_CAL_VREFANALOG;

    (void)HAL_ADCEx_Calibration_Start(s_adc, ADC_SINGLE_ENDED);

    uint16_t reload;
    uint8_t mode17;
    if (lp_wut_reload(APP_PERIOD_S, &reload, &mode17) == 0) {
        (void)HAL_RTCEx_SetWakeUpTimer_IT(s_rtc, reload,
            mode17 ? RTC_WAKEUPCLOCK_CK_SPRE_17BITS : RTC_WAKEUPCLOCK_CK_SPRE_16BITS);
    }

    char line[64];
    int n = snprintf(line, sizeof line, "lp-logger period=%lus flush=%lu\r\n",
                     (unsigned long)APP_PERIOD_S, (unsigned long)APP_FLUSH_EVERY);
    uart_print(line, n);
}

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
    if (hrtc == s_rtc) s_wut_events++;
}

void app_step(void)
{
    lp_record_t rec;
    measure(&rec);
    lp_ring_push(&s_ring, &rec);
    s_wakes++;

    if (s_wakes % APP_FLUSH_EVERY == 0u) {
        flush();
    }

    /* Sleep. SysTick would wake the core every 1 ms, so stop it first. */
    HAL_SuspendTick();
    HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);

    /* Back from Stop 2: running on MSI (MSISRANGE), so restore the full clock tree. */
    if (s_restore_clocks) s_restore_clocks();
    HAL_ResumeTick();
    s_uptime_s += APP_PERIOD_S;
}

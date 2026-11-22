#ifndef LP_MATH_H
#define LP_MATH_H

/*
 * Pure C helpers for the low-power logger. No HAL, no globals, integer only.
 * Everything here can be checked on a PC (see tools/lp_model.py, which
 * mirrors these formulas line for line).
 */
#include <stdint.h>

/* ---------- ADC conversions (12-bit) ---------- */

/* Supply voltage from the internal reference.
 * VDDA = cal_vref_mv * vrefint_cal / vrefint_raw. Returns 0 if raw is 0. */
uint32_t lp_vdda_mv(uint16_t vrefint_raw, uint16_t vrefint_cal, uint32_t cal_vref_mv);

/* ADC counts to millivolts, given the real VDDA. */
uint32_t lp_counts_to_mv(uint32_t counts, uint32_t vdda_mv);

/* Internal temperature sensor in tenths of a degree C.
 * ts_raw is rescaled from the real VDDA to the calibration VDDA first,
 * then a straight line is drawn through the two factory points. */
typedef struct {
    uint16_t cal1;        /* raw count at t1 (TS_CAL1) */
    uint16_t cal2;        /* raw count at t2 (TS_CAL2) */
    int16_t  t1_c;        /* e.g. 30  */
    int16_t  t2_c;        /* e.g. 130 */
    uint16_t cal_vref_mv; /* VDDA used at the factory, e.g. 3000 */
} lp_ts_cal_t;

int32_t lp_temp_c_x10(uint16_t ts_raw, uint32_t vdda_mv, const lp_ts_cal_t *cal);

/* ---------- RTC wake-up timer ---------- */

/* Wake-up timer clocked from ck_spre (1 Hz).
 * period_s 1..65536      -> 16-bit mode, reload = period_s - 1
 * period_s 65537..131072 -> 17-bit mode, reload = period_s - 65537
 * Returns 0 on success, -1 if period_s is out of range. */
int lp_wut_reload(uint32_t period_s, uint16_t *reload, uint8_t *mode17);

/* ---------- Record ring buffer (lives in SRAM, kept in Stop 2) ---------- */

#define LP_FLAG_VDDA_LOW  0x0001u

typedef struct {
    uint32_t t_s;        /* seconds since boot (wake count * period) */
    uint16_t vdda_mv;
    int16_t  temp_c_x10;
    uint16_t ext_mv;
    uint16_t flags;
} lp_record_t;           /* 12 bytes */

#define LP_RING_LEN 32u

typedef struct {
    lp_record_t buf[LP_RING_LEN];
    uint16_t head;       /* next slot to write */
    uint16_t count;      /* records waiting    */
    uint32_t dropped;    /* oldest records overwritten before a flush */
} lp_ring_t;

void lp_ring_init(lp_ring_t *r);
void lp_ring_push(lp_ring_t *r, const lp_record_t *rec);   /* overwrites oldest when full */
int  lp_ring_pop(lp_ring_t *r, lp_record_t *out);          /* 1 if a record was returned */

/* ---------- Current budget ---------- */

typedef struct {
    uint32_t period_s;     /* time between wake-ups                       */
    uint32_t stop_na;      /* Stop 2 + RTC current, nA                    */
    uint32_t run_ua;       /* MCU current while awake, uA                 */
    uint32_t run_us;       /* awake time per wake-up (no UART), us        */
    uint32_t sensor_ua;    /* external sensor current while powered, uA   */
    uint32_t sensor_us;    /* sensor powered time per wake-up, us         */
    uint32_t tx_ua;        /* MCU current during a UART flush, uA         */
    uint32_t tx_us;        /* duration of one flush, us                   */
    uint32_t flush_every;  /* wake-ups per flush (>= 1)                   */
} lp_budget_t;

/* Average current in nA over one wake-up period. */
uint32_t lp_avg_current_na(const lp_budget_t *b);

/* Battery life in hours: capacity_mah * derate_pct / 100 / I_avg. */
uint32_t lp_life_hours(uint32_t capacity_mah, uint32_t derate_pct, uint32_t avg_na);

#endif

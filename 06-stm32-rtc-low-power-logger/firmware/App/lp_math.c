#include "lp_math.h"

/* Signed division rounded to nearest, b must be > 0. */
static int32_t div_round_s(int32_t a, int32_t b)
{
    return (a >= 0) ? (a + b / 2) / b : -((-a + b / 2) / b);
}

uint32_t lp_vdda_mv(uint16_t vrefint_raw, uint16_t vrefint_cal, uint32_t cal_vref_mv)
{
    if (vrefint_raw == 0u) return 0u;
    return (cal_vref_mv * vrefint_cal + vrefint_raw / 2u) / vrefint_raw;
}

uint32_t lp_counts_to_mv(uint32_t counts, uint32_t vdda_mv)
{
    return (counts * vdda_mv + 2047u) / 4095u;
}

int32_t lp_temp_c_x10(uint16_t ts_raw, uint32_t vdda_mv, const lp_ts_cal_t *cal)
{
    int32_t den = (int32_t)cal->cal2 - (int32_t)cal->cal1;
    if (den <= 0 || cal->cal_vref_mv == 0u) return INT16_MIN;

    /* Bring the reading to the scale the factory used. */
    int32_t adj = (int32_t)(((uint32_t)ts_raw * vdda_mv + cal->cal_vref_mv / 2u) / cal->cal_vref_mv);
    int32_t span_x10 = ((int32_t)cal->t2_c - (int32_t)cal->t1_c) * 10;
    return (int32_t)cal->t1_c * 10 + div_round_s(span_x10 * (adj - (int32_t)cal->cal1), den);
}

int lp_wut_reload(uint32_t period_s, uint16_t *reload, uint8_t *mode17)
{
    if (period_s == 0u || period_s > 131072u) return -1;
    if (period_s <= 65536u) {
        *reload = (uint16_t)(period_s - 1u);
        *mode17 = 0u;
    } else {
        *reload = (uint16_t)(period_s - 65537u);
        *mode17 = 1u;
    }
    return 0;
}

void lp_ring_init(lp_ring_t *r)
{
    r->head = 0u;
    r->count = 0u;
    r->dropped = 0u;
}

void lp_ring_push(lp_ring_t *r, const lp_record_t *rec)
{
    r->buf[r->head] = *rec;
    r->head = (uint16_t)((r->head + 1u) % LP_RING_LEN);
    if (r->count < LP_RING_LEN) {
        r->count++;
    } else {
        r->dropped++;            /* oldest entry was just overwritten */
    }
}

int lp_ring_pop(lp_ring_t *r, lp_record_t *out)
{
    if (r->count == 0u) return 0;
    uint16_t tail = (uint16_t)((r->head + LP_RING_LEN - r->count) % LP_RING_LEN);
    *out = r->buf[tail];
    r->count--;
    return 1;
}

uint32_t lp_avg_current_na(const lp_budget_t *b)
{
    if (b->period_s == 0u) return 0u;
    uint32_t every = b->flush_every ? b->flush_every : 1u;

    /* Charge per period in pC (uA * us). The Stop 2 current is not
       subtracted during the awake time, so the result is slightly high. */
    uint64_t pc = (uint64_t)b->run_ua * b->run_us
                + (uint64_t)b->sensor_ua * b->sensor_us
                + ((uint64_t)b->tx_ua * b->tx_us + every / 2u) / every;

    /* 1 pC every second is 1 pA, so divide by period_s * 1000 to get nA. */
    uint64_t div = (uint64_t)b->period_s * 1000u;
    return b->stop_na + (uint32_t)((pc + div / 2u) / div);
}

uint32_t lp_life_hours(uint32_t capacity_mah, uint32_t derate_pct, uint32_t avg_na)
{
    if (avg_na == 0u) return 0u;
    /* mAh * pct / 100 / (nA / 1e6) = mAh * pct * 10000 / nA */
    uint64_t num = (uint64_t)capacity_mah * derate_pct * 10000u;
    return (uint32_t)((num + avg_na / 2u) / avg_na);
}

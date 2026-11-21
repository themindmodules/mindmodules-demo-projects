#include "app_math.h"

ch_stats_t app_channel_stats(const uint16_t *block, uint32_t n_scans,
                             uint32_t n_ch, uint32_t ch)
{
    ch_stats_t s = {0xFFFFu, 0u, 0u};
    uint32_t sum = 0;

    if (block == 0 || n_scans == 0 || n_ch == 0 || ch >= n_ch) {
        s.min = 0;
        return s;
    }
    for (uint32_t i = 0; i < n_scans; i++) {
        uint16_t v = block[i * n_ch + ch];
        if (v < s.min) s.min = v;
        if (v > s.max) s.max = v;
        sum += v;
    }
    s.mean_x10 = (sum * 10u + n_scans / 2u) / n_scans;
    return s;
}

uint32_t app_counts_to_mv(uint32_t counts, uint32_t vref_mv)
{
    return (counts * vref_mv + 2047u) / 4095u;
}

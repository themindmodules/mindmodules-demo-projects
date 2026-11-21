#ifndef APP_MATH_H
#define APP_MATH_H

#include <stdint.h>

/* Statistics of one channel over one DMA block, in raw ADC counts. */
typedef struct {
    uint16_t min;
    uint16_t max;
    uint32_t mean_x10; /* mean * 10, so one decimal survives without floats */
} ch_stats_t;

/*
 * block   : interleaved samples [scan0_ch0, scan0_ch1, ..., scan1_ch0, ...]
 * n_scans : number of scans in the block
 * n_ch    : channels per scan
 * ch      : which channel to analyse (0 .. n_ch-1)
 */
ch_stats_t app_channel_stats(const uint16_t *block, uint32_t n_scans,
                             uint32_t n_ch, uint32_t ch);

/* counts -> millivolts, 12-bit ADC, vref_mv is the analog reference (e.g. 3300). */
uint32_t app_counts_to_mv(uint32_t counts, uint32_t vref_mv);

#endif

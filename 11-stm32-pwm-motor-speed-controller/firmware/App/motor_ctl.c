#include "motor_ctl.h"

/* ---- encoder ---- */

int32_t enc_delta(uint32_t now, uint32_t prev)
{
    return (int32_t)(now - prev);
}

float counts_to_rpm(int32_t delta, uint32_t cpr, float dt_s)
{
    if (cpr == 0u || dt_s <= 0.0f) {
        return 0.0f;
    }
    return ((float)delta * 60.0f) / ((float)cpr * dt_s);
}

/* ---- filters and limits ---- */

float absf(float v)
{
    return (v < 0.0f) ? -v : v;
}

float clampf(float v, float lo, float hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

float lpf_step(float y, float x, float alpha)
{
    return y + alpha * (x - y);
}

float ramp_step(float current, float target, float max_step)
{
    float diff = target - current;
    if (absf(diff) <= max_step) {
        return target;
    }
    return (diff > 0.0f) ? current + max_step : current - max_step;
}

/* ---- PI ---- */

void pi_init(pi_t *pi, float kp, float ki, float out_min, float out_max)
{
    pi->kp = kp;
    pi->ki = ki;
    pi->out_min = out_min;
    pi->out_max = out_max;
    pi->integ = 0.0f;
}

void pi_reset(pi_t *pi)
{
    pi->integ = 0.0f;
}

float pi_step(pi_t *pi, float setpoint, float measured, float dt_s)
{
    float err = setpoint - measured;
    float p = pi->kp * err;
    float next = clampf(pi->integ + pi->ki * err * dt_s, pi->out_min, pi->out_max);
    float u = p + next;

    if (u > pi->out_max) {
        u = pi->out_max;
        if (err < 0.0f) {
            pi->integ = next;   /* error is pulling us back down, let it unwind */
        }
    } else if (u < pi->out_min) {
        u = pi->out_min;
        if (err > 0.0f) {
            pi->integ = next;
        }
    } else {
        pi->integ = next;
    }
    return u;
}

/* ---- PWM mapping ---- */

void pwm_compute(float duty, uint32_t period, decay_t mode,
                 uint32_t *ccr_in1, uint32_t *ccr_in2)
{
    float d = clampf(duty, -1.0f, 1.0f);
    uint32_t on = (uint32_t)(absf(d) * (float)period + 0.5f);
    if (on > period) {
        on = period;
    }

    if (on == 0u) {
        *ccr_in1 = 0u;
        *ccr_in2 = 0u;
        return;
    }

    if (mode == DECAY_FAST) {
        /* drive during 'on', coast for the rest */
        *ccr_in1 = (d > 0.0f) ? on : 0u;
        *ccr_in2 = (d > 0.0f) ? 0u : on;
    } else {
        /* one input held high, the other is high for the off time: brake */
        *ccr_in1 = (d > 0.0f) ? period : period - on;
        *ccr_in2 = (d > 0.0f) ? period - on : period;
    }
}

/* ---- faults ---- */

static uint16_t bump(uint16_t v)
{
    return (v < 0xFFFFu) ? (uint16_t)(v + 1u) : v;
}

fault_t fault_update(fault_mon_t *m, float duty, float rpm, uint16_t limit_ticks)
{
    float ad = absf(duty);

    if (ad >= FAULT_STALL_DUTY && absf(rpm) < FAULT_STALL_RPM) {
        m->stall_ticks = bump(m->stall_ticks);
    } else {
        m->stall_ticks = 0u;
    }

    if (ad >= FAULT_REV_DUTY && absf(rpm) > FAULT_REV_RPM && (duty * rpm) < 0.0f) {
        m->rev_ticks = bump(m->rev_ticks);
    } else {
        m->rev_ticks = 0u;
    }

    if (m->rev_ticks >= limit_ticks) {
        return FAULT_REVERSED;
    }
    if (m->stall_ticks >= limit_ticks) {
        return FAULT_STALL;
    }
    return FAULT_NONE;
}

/* ---- commands ---- */

static bool is_space(char c)
{
    return c == ' ' || c == '\t';
}

static char lower(char c)
{
    return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
}

bool parse_decimal(const char *s, float *out)
{
    float sign = 1.0f;
    float value = 0.0f;
    float scale = 0.1f;
    unsigned digits = 0u;
    bool frac = false;

    while (is_space(*s)) {
        s++;
    }
    if (*s == '-' || *s == '+') {
        sign = (*s == '-') ? -1.0f : 1.0f;
        s++;
    }
    for (; *s != '\0' && !is_space(*s); s++) {
        if (*s == '.' && !frac) {
            frac = true;
        } else if (*s >= '0' && *s <= '9') {
            if (++digits > 9u) {
                return false;
            }
            if (frac) {
                value += (float)(*s - '0') * scale;
                scale *= 0.1f;
            } else {
                value = value * 10.0f + (float)(*s - '0');
            }
        } else {
            return false;
        }
    }
    while (is_space(*s)) {
        s++;
    }
    if (digits == 0u || *s != '\0') {
        return false;
    }
    *out = sign * value;
    return true;
}

/* True if line starts with word followed by space or end. Sets *rest. */
static bool word_is(const char *line, const char *word, const char **rest)
{
    while (*word != '\0') {
        if (lower(*line) != *word) {
            return false;
        }
        line++;
        word++;
    }
    if (*line != '\0' && !is_space(*line)) {
        return false;
    }
    *rest = line;
    return true;
}

static bool only_spaces(const char *s)
{
    while (is_space(*s)) {
        s++;
    }
    return *s == '\0';
}

cmd_t cmd_parse(const char *line)
{
    cmd_t c = { CMD_BAD, 0.0f };
    const char *rest = line;

    while (is_space(*line)) {
        line++;
    }
    if (*line == '\0') {
        c.type = CMD_NONE;
        return c;
    }

    if (word_is(line, "s", &rest) || word_is(line, "speed", &rest)) {
        c.type = parse_decimal(rest, &c.value) ? CMD_SPEED : CMD_BAD;
    } else if (word_is(line, "open", &rest)) {
        c.type = parse_decimal(rest, &c.value) ? CMD_OPEN : CMD_BAD;
    } else if (word_is(line, "kp", &rest)) {
        c.type = (parse_decimal(rest, &c.value) && c.value >= 0.0f) ? CMD_KP : CMD_BAD;
    } else if (word_is(line, "ki", &rest)) {
        c.type = (parse_decimal(rest, &c.value) && c.value >= 0.0f) ? CMD_KI : CMD_BAD;
    } else if (word_is(line, "log", &rest)) {
        while (is_space(*rest)) {
            rest++;
        }
        if (word_is(rest, "on", &rest) && only_spaces(rest)) {
            c.type = CMD_LOG;
            c.value = 1.0f;
        } else if (word_is(rest, "off", &rest) && only_spaces(rest)) {
            c.type = CMD_LOG;
            c.value = 0.0f;
        }
    } else if (word_is(line, "stop", &rest) && only_spaces(rest)) {
        c.type = CMD_STOP;
    } else if (word_is(line, "status", &rest) && only_spaces(rest)) {
        c.type = CMD_STATUS;
    } else if (word_is(line, "clear", &rest) && only_spaces(rest)) {
        c.type = CMD_CLEAR;
    } else if ((word_is(line, "help", &rest) || word_is(line, "?", &rest)) && only_spaces(rest)) {
        c.type = CMD_HELP;
    }
    return c;
}

/* ---- formatting ---- */

unsigned fmt_fixed(char *buf, unsigned len, float v, unsigned decimals)
{
    static const uint32_t pow10[7] = { 1u, 10u, 100u, 1000u, 10000u, 100000u, 1000000u };
    char tmp[24];
    unsigned n = 0u;
    unsigned i;
    bool neg = v < 0.0f;
    uint64_t scaled;
    uint32_t whole;
    uint32_t frac;

    if (decimals > 6u) {
        decimals = 6u;
    }
    if (neg) {
        v = -v;
    }
    if (v > 4.0e9f) {
        v = 4.0e9f;
    }
    scaled = (uint64_t)((double)v * (double)pow10[decimals] + 0.5);
    whole = (uint32_t)(scaled / pow10[decimals]);
    frac = (uint32_t)(scaled % pow10[decimals]);
    if (scaled == 0u) {
        neg = false;    /* no "-0.0" */
    }

    /* build the digits backwards */
    for (i = 0u; i < decimals; i++) {
        tmp[n++] = (char)('0' + frac % 10u);
        frac /= 10u;
    }
    if (decimals > 0u) {
        tmp[n++] = '.';
    }
    do {
        tmp[n++] = (char)('0' + whole % 10u);
        whole /= 10u;
    } while (whole != 0u);
    if (neg) {
        tmp[n++] = '-';
    }

    if (n + 1u > len) {
        if (len > 0u) {
            buf[0] = '\0';
        }
        return 0u;
    }
    for (i = 0u; i < n; i++) {
        buf[i] = tmp[n - 1u - i];
    }
    buf[n] = '\0';
    return n;
}

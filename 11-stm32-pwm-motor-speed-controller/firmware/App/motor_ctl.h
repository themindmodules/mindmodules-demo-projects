/* Pure C motor control helpers. No HAL, no hardware access, so every function
   can be checked on a PC. Speeds are in rpm at the output shaft, duty is a
   signed fraction from -1.0 (full reverse) to +1.0 (full forward). */
#ifndef MOTOR_CTL_H
#define MOTOR_CTL_H

#include <stdint.h>
#include <stdbool.h>

/* ---- encoder ---- */

/* Signed change between two readings of a free running 32 bit counter.
   Correct across the 0xFFFFFFFF to 0 wrap in both directions. */
int32_t enc_delta(uint32_t now, uint32_t prev);

/* Counts seen in one window of dt_s seconds to rpm. cpr is counts per output
   revolution after x4 decoding. Returns 0 for cpr 0 or dt_s <= 0. */
float counts_to_rpm(int32_t delta, uint32_t cpr, float dt_s);

/* ---- filters and limits ---- */

/* First order low pass: y + alpha * (x - y). alpha 1.0 means no filtering. */
float lpf_step(float y, float x, float alpha);

/* Move current toward target by at most max_step. Lands exactly on target. */
float ramp_step(float current, float target, float max_step);

float clampf(float v, float lo, float hi);
float absf(float v);

/* ---- PI speed controller ---- */

typedef struct {
    float kp;       /* duty per rpm of error */
    float ki;       /* duty per rpm of error per second */
    float out_min;  /* usually -1.0 */
    float out_max;  /* usually +1.0 */
    float integ;    /* integrator state, already in duty units */
} pi_t;

void pi_init(pi_t *pi, float kp, float ki, float out_min, float out_max);
void pi_reset(pi_t *pi);

/* One controller step. The integrator only moves when the output is not
   saturated, or when the error would pull it back out of saturation
   (clamping anti-windup). */
float pi_step(pi_t *pi, float setpoint, float measured, float dt_s);

/* ---- H-bridge PWM mapping (DRV8871 style IN1/IN2 bridge) ---- */

typedef enum {
    DECAY_FAST = 0, /* off time: both inputs low, bridge coasts */
    DECAY_SLOW = 1  /* off time: both inputs high, bridge brakes */
} decay_t;

/* Turn a signed duty into compare values for the IN1 and IN2 timer channels.
   period = ARR + 1. A compare value equal to period means always high.
   Duty 0 gives 0 and 0 (coast) in both modes. */
void pwm_compute(float duty, uint32_t period, decay_t mode,
                 uint32_t *ccr_in1, uint32_t *ccr_in2);

/* ---- fault monitor ---- */

typedef enum {
    FAULT_NONE = 0,
    FAULT_STALL = 1,    /* high duty but the shaft is not turning */
    FAULT_REVERSED = 2  /* turning the wrong way: motor or encoder wires swapped */
} fault_t;

typedef struct {
    uint16_t stall_ticks;
    uint16_t rev_ticks;
} fault_mon_t;

#define FAULT_STALL_DUTY   0.50f
#define FAULT_STALL_RPM    5.0f
#define FAULT_REV_DUTY     0.30f
#define FAULT_REV_RPM      20.0f

/* Call once per control step. Returns a fault after limit_ticks steps in a row. */
fault_t fault_update(fault_mon_t *m, float duty, float rpm, uint16_t limit_ticks);

/* ---- serial commands ---- */

typedef enum {
    CMD_NONE = 0, CMD_SPEED, CMD_OPEN, CMD_STOP, CMD_KP, CMD_KI,
    CMD_LOG, CMD_STATUS, CMD_CLEAR, CMD_HELP, CMD_BAD
} cmd_type_t;

typedef struct {
    cmd_type_t type;
    float value;
} cmd_t;

/* Plain decimal such as "150", "-42.5", "0.0027". No exponents. Trailing
   spaces are allowed, anything else makes it return false. */
bool parse_decimal(const char *s, float *out);

/* One line without the line ending, for example "s 150" or "kp 0.003". */
cmd_t cmd_parse(const char *line);

/* ---- number formatting without printf float support ---- */

/* Writes v rounded to 'decimals' places (0 to 6), for example -12.36 with
   decimals 1 gives "-12.4". Returns characters written, or 0 if buf is too small. */
unsigned fmt_fixed(char *buf, unsigned len, float v, unsigned decimals);

#endif

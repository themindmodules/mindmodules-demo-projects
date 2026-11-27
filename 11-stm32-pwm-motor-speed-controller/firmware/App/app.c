/* Motor speed controller: HAL glue.
   The control loop runs in the TIM10 update interrupt. The main loop only
   reads commands and prints. All the maths lives in motor_ctl.c. */
#include "app.h"
#include "motor_ctl.h"
#include <string.h>

/* ---- settings, change these for your motor ---- */
#define LOOP_HZ          100u
#define LOOP_DT          (1.0f / (float)LOOP_HZ)
#define ENC_CPR          1320u     /* 11 pulses x 4 edges x 30:1 gearbox */
#define RPM_MAX          300.0f    /* commands above this are clamped */
#define RAMP_RPM_PER_S   600.0f    /* setpoint slew limit */
#define LPF_ALPHA        0.30f     /* speed filter, see docs/architecture.md */
#define KP_DEFAULT       0.0027f   /* duty per rpm */
#define KI_DEFAULT       0.033f    /* duty per rpm per second */
#define FAULT_TICKS      100u      /* 1 s at 100 Hz */
#define LOG_EVERY        10u       /* control steps per CSV row, 10 Hz */
#define DECAY_MODE       DECAY_SLOW
#define ZERO_RPM_BAND    2.0f

typedef enum { MODE_OFF = 0, MODE_CLOSED = 1, MODE_OPEN = 2 } run_mode_t;

static TIM_HandleTypeDef *s_pwm;
static TIM_HandleTypeDef *s_enc;
static TIM_HandleTypeDef *s_loop;
static UART_HandleTypeDef *s_uart;
static uint32_t s_period;

/* shared between the interrupt and the main loop */
static pi_t s_pi;
static fault_mon_t s_mon;
static volatile run_mode_t s_mode = MODE_OFF;
static volatile fault_t s_fault = FAULT_NONE;
static volatile float s_target;
static volatile float s_ramp;
static volatile float s_open_duty;
static volatile float s_rpm;
static volatile float s_duty;
static volatile uint32_t s_steps;
static volatile bool s_log_due;
static uint32_t s_prev_cnt;
static bool s_log_on = true;

/* serial receive, one byte at a time by interrupt */
static uint8_t s_rx_byte;
static char s_rx_buf[40];
static uint8_t s_rx_len;
static char s_line[40];
static volatile bool s_line_ready;

/* ---- output ---- */

static void put(const char *s)
{
    HAL_UART_Transmit(s_uart, (uint8_t *)s, (uint16_t)strlen(s), 50u);
}

static void put_num(float v, unsigned decimals)
{
    char b[24];
    fmt_fixed(b, sizeof b, v, decimals);
    put(b);
}

static void put_u32(uint32_t v)
{
    put_num((float)v, 0u);
}

static void apply_duty(float duty)
{
    uint32_t c1;
    uint32_t c2;
    pwm_compute(duty, s_period, DECAY_MODE, &c1, &c2);
    __HAL_TIM_SET_COMPARE(s_pwm, TIM_CHANNEL_1, c1);
    __HAL_TIM_SET_COMPARE(s_pwm, TIM_CHANNEL_2, c2);
}

static const char *fault_name(fault_t f)
{
    switch (f) {
    case FAULT_STALL:    return "stall";
    case FAULT_REVERSED: return "reversed";
    default:             return "none";
    }
}

/* ---- control loop, 100 Hz in the TIM10 interrupt ---- */

static void control_step(void)
{
    uint32_t cnt = __HAL_TIM_GET_COUNTER(s_enc);
    int32_t delta = enc_delta(cnt, s_prev_cnt);
    float rpm;
    float duty = 0.0f;

    s_prev_cnt = cnt;
    rpm = lpf_step(s_rpm, counts_to_rpm(delta, ENC_CPR, LOOP_DT), LPF_ALPHA);
    s_rpm = rpm;

    if (s_fault == FAULT_NONE) {
        if (s_mode == MODE_CLOSED) {
            s_ramp = ramp_step(s_ramp, s_target, RAMP_RPM_PER_S * LOOP_DT);
            if (s_ramp == 0.0f && absf(rpm) < ZERO_RPM_BAND) {
                pi_reset(&s_pi);    /* parked: do not let the integrator creep */
            } else {
                duty = pi_step(&s_pi, s_ramp, rpm, LOOP_DT);
            }
        } else if (s_mode == MODE_OPEN) {
            duty = s_open_duty;
        }

        s_fault = fault_update(&s_mon, duty, rpm, (uint16_t)FAULT_TICKS);
        if (s_fault != FAULT_NONE) {
            duty = 0.0f;
            s_mode = MODE_OFF;
            s_target = 0.0f;
            s_ramp = 0.0f;
            pi_reset(&s_pi);
        }
    }

    s_duty = duty;
    apply_duty(duty);

    s_steps++;
    if ((s_steps % LOG_EVERY) == 0u) {
        s_log_due = true;
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == s_loop) {
        control_step();
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart != s_uart) {
        return;
    }
    char c = (char)s_rx_byte;
    if (c == '\r' || c == '\n') {
        if (s_rx_len > 0u && !s_line_ready) {
            memcpy(s_line, s_rx_buf, s_rx_len);
            s_line[s_rx_len] = '\0';
            s_line_ready = true;
        }
        s_rx_len = 0u;
    } else if (c == 0x08 || c == 0x7F) {
        if (s_rx_len > 0u) {
            s_rx_len--;     /* backspace */
        }
    } else if (s_rx_len < sizeof s_rx_buf - 1u) {
        s_rx_buf[s_rx_len++] = c;
    }
    HAL_UART_Receive_IT(s_uart, &s_rx_byte, 1u);
}

/* ---- commands ---- */

static void print_help(void)
{
    put("commands:\r\n"
        "  s <rpm>      closed loop speed, negative for reverse\r\n"
        "  open <pct>   fixed duty -100..100, no feedback\r\n"
        "  stop         coast and reset the controller\r\n"
        "  kp <v>       proportional gain, duty per rpm\r\n"
        "  ki <v>       integral gain, duty per rpm per s\r\n"
        "  log on|off   CSV rows at 10 Hz\r\n"
        "  status       show settings and state\r\n"
        "  clear        clear a stall or reversed fault\r\n");
}

static void print_status(void)
{
    static const char *modes[] = { "off", "closed", "open" };
    put("# mode ");
    put(modes[s_mode]);
    put(" target ");
    put_num(s_target, 1u);
    put(" rpm ");
    put_num(s_rpm, 1u);
    put(" duty ");
    put_num(s_duty * 100.0f, 1u);
    put(" kp ");
    put_num(s_pi.kp, 5u);
    put(" ki ");
    put_num(s_pi.ki, 4u);
    put(" fault ");
    put(fault_name(s_fault));
    put("\r\n");
}

static void handle_line(const char *line)
{
    cmd_t c = cmd_parse(line);

    switch (c.type) {
    case CMD_NONE:
        return;
    case CMD_SPEED:
        if (s_fault != FAULT_NONE) {
            put("# fault active, send clear first\r\n");
            return;
        }
        __disable_irq();
        if (s_mode != MODE_CLOSED) {
            s_ramp = s_rpm;     /* start the ramp from where the shaft is now */
            pi_reset(&s_pi);
        }
        s_target = clampf(c.value, -RPM_MAX, RPM_MAX);
        s_mode = MODE_CLOSED;
        __enable_irq();
        break;
    case CMD_OPEN:
        if (s_fault != FAULT_NONE) {
            put("# fault active, send clear first\r\n");
            return;
        }
        __disable_irq();
        s_open_duty = clampf(c.value / 100.0f, -1.0f, 1.0f);
        s_mode = MODE_OPEN;
        __enable_irq();
        break;
    case CMD_STOP:
        __disable_irq();
        s_mode = MODE_OFF;
        s_target = 0.0f;
        s_ramp = 0.0f;
        pi_reset(&s_pi);
        __enable_irq();
        break;
    case CMD_KP:
        __disable_irq();
        s_pi.kp = c.value;
        __enable_irq();
        break;
    case CMD_KI:
        __disable_irq();
        s_pi.ki = c.value;
        __enable_irq();
        break;
    case CMD_LOG:
        s_log_on = (c.value > 0.5f);
        break;
    case CMD_CLEAR:
        __disable_irq();
        s_fault = FAULT_NONE;
        s_mon.stall_ticks = 0u;
        s_mon.rev_ticks = 0u;
        __enable_irq();
        break;
    case CMD_HELP:
        print_help();
        return;
    case CMD_STATUS:
        break;
    default:
        put("# unknown command, type help\r\n");
        return;
    }
    print_status();
}

/* ---- public ---- */

void app_init(TIM_HandleTypeDef *pwm, TIM_HandleTypeDef *enc,
              TIM_HandleTypeDef *loop, UART_HandleTypeDef *uart)
{
    s_pwm = pwm;
    s_enc = enc;
    s_loop = loop;
    s_uart = uart;
    s_period = __HAL_TIM_GET_AUTORELOAD(s_pwm) + 1u;

    pi_init(&s_pi, KP_DEFAULT, KI_DEFAULT, -1.0f, 1.0f);
    apply_duty(0.0f);

    HAL_TIM_PWM_Start(s_pwm, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(s_pwm, TIM_CHANNEL_2);
    HAL_TIM_Encoder_Start(s_enc, TIM_CHANNEL_ALL);
    s_prev_cnt = __HAL_TIM_GET_COUNTER(s_enc);
    HAL_UART_Receive_IT(s_uart, &s_rx_byte, 1u);
    HAL_TIM_Base_Start_IT(s_loop);

    put("\r\n# DC motor speed controller, encoder ");
    put_u32(ENC_CPR);
    put(" counts per rev, loop ");
    put_u32(LOOP_HZ);
    put(" Hz, PWM ");
    put_u32(84000000u / s_period);
    put(" Hz\r\n# type help for commands\r\n");
    put("ms,target_rpm,rpm,duty_pct,fault\r\n");
}

void app_poll(void)
{
    if (s_line_ready) {
        char line[sizeof s_line];
        memcpy(line, s_line, sizeof line);
        s_line_ready = false;
        handle_line(line);
    }

    if (s_log_due) {
        float target;
        float rpm;
        float duty;
        fault_t fault;

        s_log_due = false;
        __disable_irq();
        target = s_ramp;
        rpm = s_rpm;
        duty = s_duty;
        fault = s_fault;
        __enable_irq();

        if (s_log_on) {
            put_u32(HAL_GetTick());
            put(",");
            put_num(target, 1u);
            put(",");
            put_num(rpm, 1u);
            put(",");
            put_num(duty * 100.0f, 1u);
            put(",");
            put_u32((uint32_t)fault);
            put("\r\n");
        }
    }
}

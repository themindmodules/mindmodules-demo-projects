/* PC tests for motor_ctl.c. Build and run with any C compiler:
     cc -std=c11 -Wall -Wextra -I../App test_motor_ctl.c ../App/motor_ctl.c -o t && ./t
   Expected values match tools/pi_sim.py --vectors. Prints PASS or the failing lines. */
#include <stdio.h>
#include <string.h>
#include "motor_ctl.h"

static int fails;

#define CHECK(cond) do { if (!(cond)) { fails++; printf("FAIL line %d: %s\n", __LINE__, #cond); } } while (0)

static int near(float a, float b, float tol)
{
    float d = a - b;
    return (d < 0.0f ? -d : d) <= tol;
}

static void test_encoder(void)
{
    CHECK(enc_delta(5u, 0xFFFFFFFBu) == 10);
    CHECK(enc_delta(0xFFFFFFF0u, 0x10u) == -32);
    CHECK(enc_delta(1000u, 1000u) == 0);
    CHECK(near(counts_to_rpm(66, 1320u, 0.01f), 300.0f, 0.01f));
    CHECK(near(counts_to_rpm(-11, 1320u, 0.01f), -50.0f, 0.01f));
    CHECK(counts_to_rpm(10, 0u, 0.01f) == 0.0f);
    CHECK(counts_to_rpm(10, 1320u, 0.0f) == 0.0f);
}

static void test_filters(void)
{
    CHECK(near(lpf_step(100.0f, 200.0f, 0.3f), 130.0f, 0.001f));
    CHECK(near(lpf_step(100.0f, 200.0f, 1.0f), 200.0f, 0.001f));
    CHECK(ramp_step(0.0f, 150.0f, 6.0f) == 6.0f);
    CHECK(ramp_step(147.0f, 150.0f, 6.0f) == 150.0f);
    CHECK(ramp_step(10.0f, -50.0f, 6.0f) == 4.0f);
    CHECK(ramp_step(-3.0f, 0.0f, 6.0f) == 0.0f);
    CHECK(clampf(2.0f, -1.0f, 1.0f) == 1.0f);
    CHECK(clampf(-2.0f, -1.0f, 1.0f) == -1.0f);
}

static void test_pi(void)
{
    pi_t pi;
    float u;
    int i;

    pi_init(&pi, 0.0027f, 0.033f, -1.0f, 1.0f);
    CHECK(near(pi_step(&pi, 100.0f, 0.0f, 0.01f), 0.303f, 1e-5f));
    CHECK(near(pi_step(&pi, 100.0f, 50.0f, 0.01f), 0.1845f, 1e-5f));
    CHECK(near(pi_step(&pi, 100.0f, 90.0f, 0.01f), 0.0798f, 1e-5f));
    CHECK(near(pi.integ, 0.0528f, 1e-5f));
    pi_reset(&pi);
    CHECK(pi.integ == 0.0f);

    /* hard saturation: the integrator must not wind up */
    pi_init(&pi, 0.02f, 1.0f, -1.0f, 1.0f);
    for (i = 0; i < 50; i++) {
        u = pi_step(&pi, 300.0f, 0.0f, 0.01f);
    }
    CHECK(u == 1.0f);
    CHECK(pi.integ == 0.0f);
    u = pi_step(&pi, 0.0f, 300.0f, 0.01f);
    CHECK(u == -1.0f);
    CHECK(pi.integ == 0.0f);

    /* integrator never leaves the output range */
    pi_init(&pi, 0.0f, 10.0f, -1.0f, 1.0f);
    for (i = 0; i < 100; i++) {
        (void)pi_step(&pi, 100.0f, 0.0f, 0.01f);
    }
    CHECK(pi.integ <= 1.0f);
}

static void test_pwm(void)
{
    uint32_t a;
    uint32_t b;

    pwm_compute(0.5f, 4200u, DECAY_FAST, &a, &b);
    CHECK(a == 2100u && b == 0u);
    pwm_compute(0.5f, 4200u, DECAY_SLOW, &a, &b);
    CHECK(a == 4200u && b == 2100u);
    pwm_compute(-0.25f, 4200u, DECAY_FAST, &a, &b);
    CHECK(a == 0u && b == 1050u);
    pwm_compute(-0.25f, 4200u, DECAY_SLOW, &a, &b);
    CHECK(a == 3150u && b == 4200u);
    pwm_compute(1.0f, 4200u, DECAY_SLOW, &a, &b);
    CHECK(a == 4200u && b == 0u);
    pwm_compute(1.7f, 4200u, DECAY_FAST, &a, &b);
    CHECK(a == 4200u && b == 0u);
    pwm_compute(0.0001f, 4200u, DECAY_SLOW, &a, &b);
    CHECK(a == 0u && b == 0u);
    pwm_compute(0.0f, 4200u, DECAY_FAST, &a, &b);
    CHECK(a == 0u && b == 0u);
}

static void test_faults(void)
{
    fault_mon_t m = { 0u, 0u };
    int i;
    fault_t f = FAULT_NONE;

    for (i = 0; i < 99; i++) {
        f = fault_update(&m, 0.8f, 0.0f, 100u);
    }
    CHECK(f == FAULT_NONE);
    CHECK(fault_update(&m, 0.8f, 0.0f, 100u) == FAULT_STALL);

    m.stall_ticks = 0u;
    for (i = 0; i < 50; i++) {
        (void)fault_update(&m, 0.8f, 0.0f, 100u);
    }
    CHECK(fault_update(&m, 0.8f, 100.0f, 100u) == FAULT_NONE);
    CHECK(m.stall_ticks == 0u);

    for (i = 0; i < 100; i++) {
        f = fault_update(&m, 0.4f, -60.0f, 100u);
    }
    CHECK(f == FAULT_REVERSED);

    m.rev_ticks = 0u;
    for (i = 0; i < 200; i++) {
        f = fault_update(&m, 0.2f, 0.0f, 100u);    /* low duty never trips */
    }
    CHECK(f == FAULT_NONE);
}

static void test_commands(void)
{
    float v = 0.0f;
    cmd_t c;

    CHECK(parse_decimal("150", &v) && v == 150.0f);
    CHECK(parse_decimal(" -42.5 ", &v) && v == -42.5f);
    CHECK(parse_decimal("0.0027", &v) && near(v, 0.0027f, 1e-7f));
    CHECK(parse_decimal("+3", &v) && v == 3.0f);
    CHECK(!parse_decimal("", &v));
    CHECK(!parse_decimal("-", &v));
    CHECK(!parse_decimal("1.2.3", &v));
    CHECK(!parse_decimal("12abc", &v));
    CHECK(!parse_decimal("1 2", &v));
    CHECK(!parse_decimal("1e3", &v));

    c = cmd_parse("s 150");
    CHECK(c.type == CMD_SPEED && c.value == 150.0f);
    c = cmd_parse("SPEED -80");
    CHECK(c.type == CMD_SPEED && c.value == -80.0f);
    c = cmd_parse("open 35");
    CHECK(c.type == CMD_OPEN && c.value == 35.0f);
    c = cmd_parse("kp 0.004");
    CHECK(c.type == CMD_KP && near(c.value, 0.004f, 1e-7f));
    CHECK(cmd_parse("kp -1").type == CMD_BAD);
    CHECK(cmd_parse("ki 0.05").type == CMD_KI);
    c = cmd_parse("log off");
    CHECK(c.type == CMD_LOG && c.value == 0.0f);
    c = cmd_parse("log on");
    CHECK(c.type == CMD_LOG && c.value == 1.0f);
    CHECK(cmd_parse("log maybe").type == CMD_BAD);
    CHECK(cmd_parse("stop").type == CMD_STOP);
    CHECK(cmd_parse("  status  ").type == CMD_STATUS);
    CHECK(cmd_parse("clear").type == CMD_CLEAR);
    CHECK(cmd_parse("?").type == CMD_HELP);
    CHECK(cmd_parse("").type == CMD_NONE);
    CHECK(cmd_parse("   ").type == CMD_NONE);
    CHECK(cmd_parse("s").type == CMD_BAD);
    CHECK(cmd_parse("stopp").type == CMD_BAD);
    CHECK(cmd_parse("sx 10").type == CMD_BAD);
}

static void test_format(void)
{
    char b[24];

    CHECK(fmt_fixed(b, sizeof b, 148.64f, 1u) == 5u && strcmp(b, "148.6") == 0);
    CHECK(fmt_fixed(b, sizeof b, -12.36f, 1u) == 5u && strcmp(b, "-12.4") == 0);
    CHECK(fmt_fixed(b, sizeof b, 0.0027f, 5u) && strcmp(b, "0.00270") == 0);
    CHECK(fmt_fixed(b, sizeof b, -0.01f, 1u) && strcmp(b, "0.0") == 0);
    CHECK(fmt_fixed(b, sizeof b, 12345.0f, 0u) && strcmp(b, "12345") == 0);
    CHECK(fmt_fixed(b, 4u, 12345.0f, 0u) == 0u && b[0] == '\0');
}

int main(void)
{
    test_encoder();
    test_filters();
    test_pi();
    test_pwm();
    test_faults();
    test_commands();
    test_format();
    if (fails == 0) {
        printf("PASS\n");
        return 0;
    }
    printf("%d check(s) failed\n", fails);
    return 1;
}

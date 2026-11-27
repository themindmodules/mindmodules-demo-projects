"""Step response simulation of the speed loop, plus a Python copy of motor_ctl.c.

The motor is a first order model with a friction dead band. It is a guess
for a small 12 V gear motor, not a measured motor. Use it to see how the
gains, the filter and the encoder resolution interact before you touch real
hardware, then retune on the bench.

  python pi_sim.py                 summary and a table every 100 ms
  python pi_sim.py --kp 0.004      try other gains
  python pi_sim.py --csv out.csv   full 1 kHz trace for a spreadsheet
  python pi_sim.py --vectors       print the values used in test_motor_ctl.c

Plain Python 3, no packages needed.
"""
import argparse
import math

# ---------- copy of motor_ctl.c ----------


def enc_delta(now, prev):
    d = (now - prev) & 0xFFFFFFFF
    return d - 0x100000000 if d >= 0x80000000 else d


def counts_to_rpm(delta, cpr, dt):
    if cpr == 0 or dt <= 0:
        return 0.0
    return delta * 60.0 / (cpr * dt)


def clampf(v, lo, hi):
    return lo if v < lo else hi if v > hi else v


def lpf_step(y, x, alpha):
    return y + alpha * (x - y)


def ramp_step(cur, target, step):
    diff = target - cur
    if abs(diff) <= step:
        return target
    return cur + step if diff > 0 else cur - step


class PI:
    def __init__(self, kp, ki, lo=-1.0, hi=1.0):
        self.kp, self.ki, self.lo, self.hi, self.integ = kp, ki, lo, hi, 0.0

    def reset(self):
        self.integ = 0.0

    def step(self, sp, meas, dt):
        err = sp - meas
        p = self.kp * err
        nxt = clampf(self.integ + self.ki * err * dt, self.lo, self.hi)
        u = p + nxt
        if u > self.hi:
            u = self.hi
            if err < 0:
                self.integ = nxt
        elif u < self.lo:
            u = self.lo
            if err > 0:
                self.integ = nxt
        else:
            self.integ = nxt
        return u


def pwm_compute(duty, period, slow):
    d = clampf(duty, -1.0, 1.0)
    on = min(int(abs(d) * period + 0.5), period)
    if on == 0:
        return 0, 0
    if not slow:
        return (on, 0) if d > 0 else (0, on)
    return (period, period - on) if d > 0 else (period - on, period)


# ---------- motor model ----------

S1 = 160.0   # first step, rpm (not a whole number of counts per window)
S2 = 240.0   # second step, rpm

class Motor:
    """d(rpm)/dt = (K * u_eff - rpm - load) / tau"""

    def __init__(self, k_rpm=300.0, tau=0.08, deadband=0.08, cpr=1320):
        self.k, self.tau, self.db, self.cpr = k_rpm, tau, deadband, cpr
        self.rpm = 0.0
        self.pos = 0.0      # in counts, fractional
        self.load_rpm = 0.0  # load expressed as rpm lost at the shaft

    def step(self, duty, dt):
        a = abs(duty)
        u = 0.0 if a < self.db else math.copysign((a - self.db) / (1 - self.db), duty)
        drive = self.k * u
        if abs(self.rpm) > 0.5:          # load only acts while the shaft turns
            drive -= math.copysign(self.load_rpm, self.rpm)
        self.rpm += (drive - self.rpm) / self.tau * dt
        self.pos += self.rpm / 60.0 * self.cpr * dt

    def counter(self):
        return int(math.floor(self.pos)) & 0xFFFFFFFF


def simulate(kp, ki, alpha=0.30, loop_hz=100, ramp=600.0, cpr=1320, tau=0.08, k_rpm=300.0):
    m = Motor(k_rpm=k_rpm, tau=tau, cpr=cpr)
    pi = PI(kp, ki)
    dt_loop = 1.0 / loop_hz
    sub = 10                      # motor solved at 10x the loop rate
    events = [(0.2, S1, 0.0), (1.5, S2, 0.0), (2.5, S2, 40.0), (3.5, 0.0, 40.0)]
    t_end = 4.5
    target = ramp_v = rpm_f = duty = 0.0
    prev = m.counter()
    rows = []
    n = int(round(t_end * loop_hz))
    for i in range(n):
        t = i * dt_loop
        for (te, sp, load) in events:
            if abs(t - te) < 1e-9:
                target, m.load_rpm = sp, load
        cnt = m.counter()
        raw = counts_to_rpm(enc_delta(cnt, prev), cpr, dt_loop)
        prev = cnt
        rpm_f = lpf_step(rpm_f, raw, alpha)
        ramp_v = ramp_step(ramp_v, target, ramp * dt_loop)
        if ramp_v == 0.0 and abs(rpm_f) < 2.0:
            pi.reset()
            duty = 0.0
        else:
            duty = pi.step(ramp_v, rpm_f, dt_loop)
        rows.append((t, target, ramp_v, m.rpm, rpm_f, duty))
        for _ in range(sub):
            m.step(duty, dt_loop / sub)
    return rows


def window(rows, t0, t1):
    return [r for r in rows if t0 <= r[0] < t1]


def metrics(rows):
    out = {}
    step = window(rows, 0.2, 1.5)
    t10 = next(r[0] for r in step if r[3] >= 0.1 * S1)
    t90 = next(r[0] for r in step if r[3] >= 0.9 * S1)
    out["rise_10_90_s"] = t90 - t10
    out["overshoot_pct"] = max(0.0, (max(r[3] for r in step) - S1) / S1 * 100.0)
    settle = None
    for r in step:
        if abs(r[3] - S1) > 0.02 * S1:
            settle = None
        elif settle is None:
            settle = r[0]
    out["settle_2pct_s"] = (settle - 0.2) if settle is not None else float("nan")
    for name, (a, b, sp) in {"err_step1": (1.2, 1.5, S1), "err_step2": (2.2, 2.5, S2), "err_step2_load": (3.2, 3.5, S2)}.items():
        w = window(rows, a, b)
        out[name] = sum(r[3] for r in w) / len(w) - sp
    load = window(rows, 2.5, 3.5)
    out["load_dip_rpm"] = S2 - min(r[3] for r in load)
    out["max_duty_pct"] = max(abs(r[5]) for r in rows) * 100.0
    w = window(rows, 2.2, 2.5)
    out["meas_ripple_rpm"] = max(r[4] for r in w) - min(r[4] for r in w)
    out["duty_ripple_pct"] = (max(r[5] for r in w) - min(r[5] for r in w)) * 100.0
    end = window(rows, 4.3, 4.5)
    out["end_rpm"] = sum(abs(r[3]) for r in end) / len(end)
    return out


def vectors():
    print("enc_delta(5, 0xFFFFFFFB) =", enc_delta(5, 0xFFFFFFFB))
    print("enc_delta(0xFFFFFFF0, 0x10) =", enc_delta(0xFFFFFFF0, 0x10))
    print("counts_to_rpm(66, 1320, 0.01) = %.4f" % counts_to_rpm(66, 1320, 0.01))
    print("counts_to_rpm(-11, 1320, 0.01) = %.4f" % counts_to_rpm(-11, 1320, 0.01))
    print("lpf_step(100, 200, 0.3) = %.4f" % lpf_step(100, 200, 0.3))
    print("ramp_step(0, 150, 6) = %.4f, ramp_step(147, 150, 6) = %.4f" % (ramp_step(0, 150, 6), ramp_step(147, 150, 6)))
    pi = PI(0.0027, 0.033)
    seq = [pi.step(100.0, 0.0, 0.01), pi.step(100.0, 50.0, 0.01), pi.step(100.0, 90.0, 0.01)]
    print("PI(0.0027, 0.033) steps sp=100 meas 0,50,90 -> %.6f %.6f %.6f integ %.6f" % (*seq, pi.integ))
    pi = PI(0.02, 1.0)
    for _ in range(50):
        pi.step(300.0, 0.0, 0.01)
    print("saturated PI(0.02,1.0) 50 steps sp=300 meas 0: integ %.4f" % pi.integ)
    u = pi.step(0.0, 300.0, 0.01)
    print("  then sp=0 meas=300: u %.4f integ %.4f" % (u, pi.integ))
    for d in (0.5, -0.25, 1.0, 0.0001):
        print("pwm_compute(%g, 4200) fast=%s slow=%s" % (d, pwm_compute(d, 4200, False), pwm_compute(d, 4200, True)))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--kp", type=float, default=0.0027)
    ap.add_argument("--ki", type=float, default=0.033)
    ap.add_argument("--alpha", type=float, default=0.30)
    ap.add_argument("--cpr", type=int, default=1320)
    ap.add_argument("--tau", type=float, default=0.08)
    ap.add_argument("--csv")
    ap.add_argument("--vectors", action="store_true")
    a = ap.parse_args()
    if a.vectors:
        vectors()
        return
    rows = simulate(a.kp, a.ki, alpha=a.alpha, cpr=a.cpr, tau=a.tau)
    print("kp %.5f  ki %.4f  alpha %.2f  cpr %d  motor tau %.3f s" % (a.kp, a.ki, a.alpha, a.cpr, a.tau))
    print("   t   target  ramp    rpm   rpm_meas  duty%")
    for r in rows[::10]:
        print("%5.2f %7.1f %6.1f %7.1f %8.1f %7.1f" % (r[0], r[1], r[2], r[3], r[4], r[5] * 100))
    print()
    for k, v in metrics(rows).items():
        print("%-16s %8.3f" % (k, v))
    if a.csv:
        with open(a.csv, "w") as f:
            f.write("t,target,ramp,rpm,rpm_meas,duty\n")
            for r in rows:
                f.write("%.3f,%.1f,%.2f,%.3f,%.3f,%.5f\n" % r)


if __name__ == "__main__":
    main()

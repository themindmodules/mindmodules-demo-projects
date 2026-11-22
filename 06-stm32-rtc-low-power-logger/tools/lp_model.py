"""PC model of firmware/App/lp_math.c.

The integer formulas here follow the C code line for line, so the numbers in
docs/power-budget.md can be regenerated and the edge cases checked without a board.

  python lp_model.py            run the self test, then print the budget tables
  python lp_model.py --test     self test only
"""
import sys

LP_RING_LEN = 32


def div_round_s(a, b):
    # C integer division truncates toward zero; mirror that exactly
    return (a + b // 2) // b if a >= 0 else -((-a + b // 2) // b)


def vdda_mv(raw, cal, cal_vref=3000):
    return 0 if raw == 0 else (cal_vref * cal + raw // 2) // raw


def counts_to_mv(counts, vdda):
    return (counts * vdda + 2047) // 4095


def temp_c_x10(ts_raw, vdda, cal1, cal2, t1=30, t2=130, cal_vref=3000):
    den = cal2 - cal1
    if den <= 0:
        return -32768
    adj = (ts_raw * vdda + cal_vref // 2) // cal_vref
    return t1 * 10 + div_round_s((t2 - t1) * 10 * (adj - cal1), den)


def wut_reload(period_s):
    if period_s == 0 or period_s > 131072:
        return None
    return (period_s - 1, 0) if period_s <= 65536 else (period_s - 65537, 1)


class Ring:
    def __init__(self):
        self.buf = [None] * LP_RING_LEN
        self.head = self.count = self.dropped = 0

    def push(self, rec):
        self.buf[self.head] = rec
        self.head = (self.head + 1) % LP_RING_LEN
        if self.count < LP_RING_LEN:
            self.count += 1
        else:
            self.dropped += 1

    def pop(self):
        if self.count == 0:
            return None
        tail = (self.head + LP_RING_LEN - self.count) % LP_RING_LEN
        self.count -= 1
        return self.buf[tail]


def avg_current_na(period_s, stop_na, run_ua, run_us, sensor_ua, sensor_us, tx_ua, tx_us, flush_every):
    every = flush_every or 1
    pc = run_ua * run_us + sensor_ua * sensor_us + (tx_ua * tx_us + every // 2) // every
    div = period_s * 1000
    return stop_na + (pc + div // 2) // div


def life_hours(capacity_mah, derate_pct, avg_na):
    return 0 if avg_na == 0 else (capacity_mah * derate_pct * 10000 + avg_na // 2) // avg_na


def self_test():
    checks = 0

    def eq(got, want, what):
        nonlocal checks
        checks += 1
        if got != want:
            raise AssertionError("%s: got %r, want %r" % (what, got, want))

    # VDDA: if raw equals cal, VDDA equals the calibration voltage
    eq(vdda_mv(1655, 1655), 3000, "vdda at cal point")
    eq(vdda_mv(1505, 1655), 3299, "vdda at 3.3 V")        # 3000*1655/1505 = 3299.0
    eq(vdda_mv(2069, 1655), 2400, "vdda at 2.4 V")        # 2399.7 rounds to 2400
    eq(vdda_mv(0, 1655), 0, "vdda raw 0 guard")
    # counts to mV
    eq(counts_to_mv(0, 3300), 0, "0 counts")
    eq(counts_to_mv(4095, 3300), 3300, "full scale")
    eq(counts_to_mv(2048, 3000), 1500, "mid scale")
    # temperature: at the two cal points with VDDA = 3000 we must land on 30.0 and 130.0
    eq(temp_c_x10(1030, 3000, 1030, 1370), 300, "temp at cal1")
    eq(temp_c_x10(1370, 3000, 1030, 1370), 1300, "temp at cal2")
    eq(temp_c_x10(1200, 3000, 1030, 1370), 800, "temp midpoint")
    # same die temperature read at VDDA 3300: raw is smaller by 3000/3300
    eq(temp_c_x10(936, 3300, 1030, 1370), 300, "temp rescaled for VDDA")
    eq(temp_c_x10(950, 3000, 1030, 1370), 65, "temp below cal1")    # 30 + 100*(-80)/340 = 6.47
    eq(temp_c_x10(1000, 3000, 1100, 1100), -32768, "temp bad cal guard")
    # wake-up timer
    eq(wut_reload(1), (0, 0), "wut 1 s")
    eq(wut_reload(60), (59, 0), "wut 60 s")
    eq(wut_reload(65536), (65535, 0), "wut 16-bit max")
    eq(wut_reload(65537), (0, 1), "wut 17-bit min")
    eq(wut_reload(131072), (65535, 1), "wut 17-bit max")
    eq(wut_reload(0), None, "wut 0 rejected")
    eq(wut_reload(131073), None, "wut too long rejected")
    # ring buffer: fill past the end, oldest must be dropped and order kept
    r = Ring()
    for i in range(LP_RING_LEN + 5):
        r.push(i)
    eq(r.count, LP_RING_LEN, "ring full count")
    eq(r.dropped, 5, "ring dropped")
    out = []
    while True:
        v = r.pop()
        if v is None:
            break
        out.append(v)
    eq(out, list(range(5, LP_RING_LEN + 5)), "ring order after wrap")
    # budget: no activity at all leaves only the Stop 2 current
    eq(avg_current_na(60, 1300, 0, 0, 0, 0, 0, 0, 16), 1300, "budget idle")
    eq(life_hours(1000, 100, 1_000_000), 1000, "life 1 mA from 1000 mAh")
    print("self test: %d checks passed" % checks)


# Default scenario, see docs/power-budget.md for where each number comes from
BASE = dict(stop_na=1300, run_ua=2000, run_us=3000, sensor_ua=150, sensor_us=1500,
            tx_ua=2000, tx_us=74000, flush_every=16)
CELLS = [("CR2032", 225, 70), ("2 x AAA", 1000, 80), ("2 x AA", 2500, 80)]


def tables():
    print("\nCharge per wake-up and per period (default scenario, period 60 s)")
    b = BASE
    parts = [("Awake, measuring", b["run_ua"] * b["run_us"]),
             ("Sensor divider on", b["sensor_ua"] * b["sensor_us"]),
             ("UART flush / %d" % b["flush_every"], b["tx_ua"] * b["tx_us"] // b["flush_every"]),
             ("Stop 2 + RTC", b["stop_na"] * 60 * 1000)]
    total = sum(p for _, p in parts)
    for name, pc in parts:
        print("  %-20s %9.2f uC  %5.1f %%" % (name, pc / 1e6, 100.0 * pc / total))

    print("\nPeriod   I_avg      " + "  ".join("%-16s" % c[0] for c in CELLS))
    for period in (1, 10, 60, 300, 3600):
        ia = avg_current_na(period, **b)
        cols = []
        for _, mah, der in CELLS:
            h = life_hours(mah, der, ia)
            cols.append("%7d h %5.1f y" % (h, h / 8766.0))
        print("%5d s  %6d nA  " % (period, ia) + "  ".join("%-16s" % c for c in cols))


if __name__ == "__main__":
    self_test()
    if "--test" not in sys.argv:
        tables()

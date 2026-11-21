// Pure control logic for the plant monitor. No Arduino calls in here,
// so it can be compile-checked and unit-tested on a PC.
#ifndef PLANT_LOGIC_H
#define PLANT_LOGIC_H

#include <stdint.h>

struct PlantConfig {
  int      dryRaw;         // ADC reading with the sensor in dry air (calibrate!)
  int      wetRaw;         // ADC reading with the sensor in a glass of water (calibrate!)
  uint8_t  onBelowPct;     // start watering below this moisture %
  uint8_t  offAbovePct;    // stop watering above this moisture %
  uint32_t maxPumpMs;      // hard limit for one watering run
  uint32_t lockoutMs;      // minimum time between two runs
};

struct PlantState {
  bool     pumpOn;
  uint32_t pumpStartMs;
  uint32_t lastRunEndMs;
  bool     everRan;
  uint8_t  lastPct;
};

inline void plantInit(PlantState &s) {
  s.pumpOn = false;
  s.pumpStartMs = 0;
  s.lastRunEndMs = 0;
  s.everRan = false;
  s.lastPct = 0;
}

// Raw ADC -> 0..100 % moisture. Capacitive sensors read HIGH when dry.
inline uint8_t rawToPercent(int raw, int dryRaw, int wetRaw) {
  if (dryRaw == wetRaw) return 0;
  long num = (long)(dryRaw - raw) * 100L;
  long den = (long)(dryRaw - wetRaw);
  long pct = num / den;
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return (uint8_t)pct;
}

// One control step. Returns true if the pump should be ON.
// 'now' is millis(). Unsigned subtraction keeps it correct across millis() rollover.
inline bool plantStep(PlantState &s, const PlantConfig &c, int raw, uint32_t now) {
  s.lastPct = rawToPercent(raw, c.dryRaw, c.wetRaw);

  if (s.pumpOn) {
    bool tooLong = (uint32_t)(now - s.pumpStartMs) >= c.maxPumpMs;
    bool wetEnough = s.lastPct >= c.offAbovePct;
    if (tooLong || wetEnough) {
      s.pumpOn = false;
      s.lastRunEndMs = now;
      s.everRan = true;
    }
  } else {
    bool dry = s.lastPct < c.onBelowPct;
    bool lockedOut = s.everRan && (uint32_t)(now - s.lastRunEndMs) < c.lockoutMs;
    if (dry && !lockedOut) {
      s.pumpOn = true;
      s.pumpStartMs = now;
    }
  }
  return s.pumpOn;
}

#endif

// Pure logic for the cable tester: no Arduino calls, so it can be compile-checked and tested on a PC.
//
// A scan drives one wire at a time from the near end and reads all 8 far-end lines.
// rows[i] is the far-end bit pattern seen while wire i (0..7) was driven.
// For a good straight cable rows[i] == (1 << i).
#ifndef TESTER_LOGIC_H
#define TESTER_LOGIC_H

#include <stdint.h>

static const uint8_t MAX_WIRES = 8;

enum WireStatus : uint8_t {
  WIRE_OK = 0,    // arrives at the same pin, nowhere else
  WIRE_OPEN,      // arrives nowhere
  WIRE_CROSSED,   // arrives at exactly one other pin
  WIRE_SHORT      // arrives at two or more pins (shorted to another wire)
};

enum CableVerdict : uint8_t {
  CABLE_PASS = 0, // every wire OK
  CABLE_REVERSED, // every wire lands mirrored (1 to n, 2 to n-1, ...), like a rollover cable
  CABLE_FAIL      // anything else
};

inline uint8_t wireMask(uint8_t wires) {
  if (wires >= MAX_WIRES) return 0xFF;
  return (uint8_t)((1u << wires) - 1u);
}

inline uint8_t bitCount(uint8_t v) {
  uint8_t n = 0;
  while (v) { n += v & 1u; v >>= 1; }
  return n;
}

// Status of wire i. Far-end pins above the selected wire count are ignored.
inline WireStatus wireStatus(const uint8_t rows[MAX_WIRES], uint8_t i, uint8_t wires) {
  uint8_t seen = rows[i] & wireMask(wires);
  if (seen == 0) return WIRE_OPEN;
  if (bitCount(seen) > 1) return WIRE_SHORT;
  if (seen == (uint8_t)(1u << i)) return WIRE_OK;
  return WIRE_CROSSED;
}

inline CableVerdict cableVerdict(const uint8_t rows[MAX_WIRES], uint8_t wires) {
  if (wires == 0 || wires > MAX_WIRES) return CABLE_FAIL;
  bool allOk = true;
  bool mirrored = wires >= 2;
  uint8_t mask = wireMask(wires);
  for (uint8_t i = 0; i < wires; i++) {
    uint8_t seen = rows[i] & mask;
    if (seen != (uint8_t)(1u << i)) allOk = false;
    if (seen != (uint8_t)(1u << (wires - 1 - i))) mirrored = false;
  }
  if (allOk) return CABLE_PASS;
  if (mirrored) return CABLE_REVERSED;
  return CABLE_FAIL;
}

// Bit pattern for the 8 wire LEDs.
//   OK: on, OPEN: off, CROSSED or SHORT: blinks (on when blinkOn is true).
// For a reversed cable the per-wire LEDs would all blink, which hides the useful message,
// so the caller shows the verdict LEDs instead (see verdictLeds).
inline uint8_t wireLeds(const uint8_t rows[MAX_WIRES], uint8_t wires, bool blinkOn) {
  uint8_t out = 0;
  for (uint8_t i = 0; i < wires && i < MAX_WIRES; i++) {
    WireStatus s = wireStatus(rows, i, wires);
    if (s == WIRE_OK || ((s == WIRE_CROSSED || s == WIRE_SHORT) && blinkOn)) out |= (uint8_t)(1u << i);
  }
  return out;
}

struct VerdictLeds { bool pass; bool fail; };

// PASS: green steady. REVERSED: green blinking. FAIL: red steady.
inline VerdictLeds verdictLeds(CableVerdict v, bool blinkOn) {
  VerdictLeds l = {false, false};
  if (v == CABLE_PASS) l.pass = true;
  else if (v == CABLE_REVERSED) l.pass = blinkOn;
  else l.fail = true;
  return l;
}

// Nothing plugged in at all: every row reads zero. Show nothing instead of a wall of red.
inline bool nothingConnected(const uint8_t rows[MAX_WIRES], uint8_t wires) {
  uint8_t mask = wireMask(wires);
  for (uint8_t i = 0; i < wires && i < MAX_WIRES; i++)
    if (rows[i] & mask) return false;
  return true;
}

// Button cycles the wire count 2, 3, ... 8, then back to 2.
inline uint8_t nextWireCount(uint8_t wires) {
  return (wires >= MAX_WIRES || wires < 2) ? 2 : (uint8_t)(wires + 1);
}

// Simple time-based debounce. Returns true once per press (on the falling edge after it is stable).
struct Debounce {
  bool stable;      // last stable level (true = released, pull-up)
  bool lastRaw;
  uint32_t changedAt;
};

inline void debounceInit(Debounce &d) { d.stable = true; d.lastRaw = true; d.changedAt = 0; }

inline bool debouncePressed(Debounce &d, bool rawLevel, uint32_t nowMs, uint32_t holdMs = 30) {
  if (rawLevel != d.lastRaw) { d.lastRaw = rawLevel; d.changedAt = nowMs; }
  if (rawLevel != d.stable && (uint32_t)(nowMs - d.changedAt) >= holdMs) {
    d.stable = rawLevel;
    return !rawLevel;   // pressed = pulled low
  }
  return false;
}

#endif

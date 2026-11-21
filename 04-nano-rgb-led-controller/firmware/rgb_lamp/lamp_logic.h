// Pure logic for the RGB lamp: no Arduino calls, so it can be compile-checked and tested on a PC.
#ifndef LAMP_LOGIC_H
#define LAMP_LOGIC_H

#include <stdint.h>

enum LampMode : uint8_t { MODE_OFF = 0, MODE_WARM, MODE_RAINBOW, MODE_BREATHE, MODE_COUNT };

struct Rgb { uint8_t r, g, b; };

// Integer HSV -> RGB. h: 0..359, s,v: 0..255
inline Rgb hsvToRgb(uint16_t h, uint8_t s, uint8_t v) {
  h %= 360;
  uint8_t region = h / 60;
  uint16_t rem = (h % 60) * 255 / 60;           // 0..255 inside the region
  uint8_t p = (uint16_t)v * (255 - s) / 255;
  uint8_t q = (uint16_t)v * (255 - ((uint16_t)s * rem) / 255) / 255;
  uint8_t t = (uint16_t)v * (255 - ((uint16_t)s * (255 - rem)) / 255) / 255;
  switch (region) {
    case 0:  return {v, t, p};
    case 1:  return {q, v, p};
    case 2:  return {p, v, t};
    case 3:  return {p, q, v};
    case 4:  return {t, p, v};
    default: return {v, p, q};
  }
}

// Pot reading (0..1023) -> brightness 0..255 with a squared curve (eyes see brightness non-linearly).
inline uint8_t potToBrightness(int raw) {
  if (raw < 0) raw = 0;
  if (raw > 1023) raw = 1023;
  uint32_t lin = (uint32_t)raw * 255UL / 1023UL;
  return (uint8_t)((lin * lin) / 255UL);
}

inline LampMode nextMode(LampMode m) {
  return (LampMode)((m + 1) % MODE_COUNT);
}

// Triangle wave 0..255..0 with the given period in ms.
inline uint8_t triangle(uint32_t nowMs, uint32_t periodMs) {
  uint32_t ph = nowMs % periodMs;
  uint32_t half = periodMs / 2;
  uint32_t x = ph < half ? ph : periodMs - ph;
  return (uint8_t)(x * 255UL / half);
}

// Colour for the current mode. brightness is the master 0..255 from the pot.
inline Rgb lampColor(LampMode m, uint32_t nowMs, uint8_t brightness) {
  switch (m) {
    case MODE_WARM:    return hsvToRgb(30, 200, brightness);
    case MODE_RAINBOW: return hsvToRgb((nowMs / 40) % 360, 255, brightness);
    case MODE_BREATHE: {
      uint8_t pulse = triangle(nowMs, 4000);
      return hsvToRgb(200, 255, (uint16_t)brightness * pulse / 255);
    }
    default:           return {0, 0, 0};
  }
}

// Button debouncer: call update() every loop with the raw "pressed" state. Returns true once per press.
struct Debounce {
  bool stable = false;
  bool lastRaw = false;
  uint32_t changedAt = 0;
  bool update(bool raw, uint32_t nowMs, uint32_t holdMs = 30) {
    if (raw != lastRaw) { lastRaw = raw; changedAt = nowMs; }
    if ((uint32_t)(nowMs - changedAt) >= holdMs && raw != stable) {
      stable = raw;
      return stable;           // true only on the press edge
    }
    return false;
  }
};

#endif

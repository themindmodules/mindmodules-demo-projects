// 12 V RGB LED strip controller on an Arduino Nano.
// Pot on A0 = master brightness. Button on D2 (to GND) = next mode.
// Modes: OFF, warm white, rainbow, breathing blue.
// PWM outputs: D9 = red, D10 = green, D11 = blue (each drives an N-MOSFET gate).
// The strip must be a COMMON-ANODE (+12 V common) type.

#include "lamp_logic.h"

const uint8_t PIN_R = 9;
const uint8_t PIN_G = 10;
const uint8_t PIN_B = 11;
const uint8_t PIN_BTN = 2;
const uint8_t PIN_POT = A0;

LampMode mode = MODE_WARM;
Debounce btn;

void setup() {
  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  pinMode(PIN_BTN, INPUT_PULLUP);
  analogWrite(PIN_R, 0);
  analogWrite(PIN_G, 0);
  analogWrite(PIN_B, 0);
}

void loop() {
  uint32_t now = millis();

  if (btn.update(digitalRead(PIN_BTN) == LOW, now)) {
    mode = nextMode(mode);
  }

  uint8_t brightness = potToBrightness(analogRead(PIN_POT));
  Rgb c = lampColor(mode, now, brightness);

  analogWrite(PIN_R, c.r);
  analogWrite(PIN_G, c.g);
  analogWrite(PIN_B, c.b);
  delay(5);
}

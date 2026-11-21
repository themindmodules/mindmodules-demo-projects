// Arduino plant monitor: soil moisture + light logging, safe automatic watering.
// Board: Arduino Uno / Nano (5 V, ATmega328P). No external libraries needed.
//
// Wiring (see ../../hardware/wiring.svg):
//   A0  <- capacitive soil moisture sensor (analog out)
//   A1  <- LDR divider (LDR to 5V, 10k to GND, junction to A1)
//   D6  -> gate of logic-level N-MOSFET (pump low side), 100 ohm in series
//   D13 -> onboard LED shows "pump ON"
//   D2  <- push button to GND (hold at boot to run the calibration printout)

#include "plant_logic.h"

const uint8_t PIN_SOIL   = A0;
const uint8_t PIN_LIGHT  = A1;
const uint8_t PIN_PUMP   = 6;
const uint8_t PIN_LED    = 13;
const uint8_t PIN_BUTTON = 2;

// CALIBRATE THESE: run with the button held at power-up, read the printout.
PlantConfig cfg = {
  /* dryRaw      */ 620,
  /* wetRaw      */ 300,
  /* onBelowPct  */ 30,
  /* offAbovePct */ 45,
  /* maxPumpMs   */ 10UL * 1000UL,        // never run longer than 10 s at once
  /* lockoutMs   */ 30UL * 60UL * 1000UL  // at least 30 min between runs
};

PlantState st;

const uint8_t N_AVG = 8;
int ring[N_AVG];
uint8_t ringIdx = 0;
bool ringFull = false;

int readSoilAveraged() {
  ring[ringIdx++] = analogRead(PIN_SOIL);
  if (ringIdx >= N_AVG) { ringIdx = 0; ringFull = true; }
  uint8_t n = ringFull ? N_AVG : ringIdx;
  long sum = 0;
  for (uint8_t i = 0; i < n; i++) sum += ring[i];
  return (int)(sum / n);
}

void setup() {
  pinMode(PIN_PUMP, OUTPUT);
  digitalWrite(PIN_PUMP, LOW);          // pump definitely off at boot
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  Serial.begin(115200);
  plantInit(st);

  if (digitalRead(PIN_BUTTON) == LOW) { // calibration mode, pump stays OFF
    Serial.println(F("CALIBRATION MODE: note dry-air and water readings"));
    while (true) {
      Serial.print(F("raw=")); Serial.println(analogRead(PIN_SOIL));
      delay(500);
    }
  }
  Serial.println(F("ms,raw,moisture_pct,light_raw,pump"));
}

void loop() {
  static uint32_t lastTick = 0;
  uint32_t now = millis();
  if ((uint32_t)(now - lastTick) < 1000UL) return;   // 1 Hz control loop
  lastTick = now;

  int raw = readSoilAveraged();
  bool pump = plantStep(st, cfg, raw, now);
  digitalWrite(PIN_PUMP, pump ? HIGH : LOW);
  digitalWrite(PIN_LED, pump ? HIGH : LOW);

  Serial.print(now);            Serial.print(',');
  Serial.print(raw);            Serial.print(',');
  Serial.print(st.lastPct);     Serial.print(',');
  Serial.print(analogRead(PIN_LIGHT)); Serial.print(',');
  Serial.println(pump ? 1 : 0);
}

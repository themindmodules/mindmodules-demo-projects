// 8-wire cable continuity tester. Arduino Nano, 2x 74HC595, 1x 74HC165.
// Plug both ends of the cable into J1 (near) and J2 (far). The tester scans about 5 times a second.
// Wire LEDs: on = good, off = open, blinking = crossed or shorted.
// Green steady = pass, green blinking = reversed (rollover), red = fault.
// Button: change how many wires the cable has (2..8). The LEDs show the count for one second.

#include "tester_logic.h"

const uint8_t PIN_BTN = 2;
const uint8_t PIN_PASS = 3;
const uint8_t PIN_FAIL = 4;
const uint8_t PIN_RX_LOAD = 9;   // 74HC165 /PL
const uint8_t PIN_LATCH = 10;    // both 74HC595 RCLK
const uint8_t PIN_SER = 11;      // 74HC595 SER (drive register, chained into LED register)
const uint8_t PIN_RX_DATA = 12;  // 74HC165 Q7
const uint8_t PIN_SCK = 13;      // shared clock for all three registers

const uint16_t SETTLE_US = 100;  // cable capacitance and pulldown settle time per wire
const uint16_t SCAN_MS = 200;
const uint16_t BLINK_MS = 250;
const uint16_t SHOW_COUNT_MS = 1000;

uint8_t wires = 8;
uint8_t ledByte = 0;
uint8_t rows[MAX_WIRES];
Debounce btn;
uint32_t lastScan = 0;
uint32_t showCountUntil = 0;

// Shift the LED byte and drive byte out, then latch both registers together.
// The LED register is second in the chain, so its byte goes first.
void writeRegisters(uint8_t leds, uint8_t drive) {
  shiftOut(PIN_SER, PIN_SCK, MSBFIRST, leds);
  shiftOut(PIN_SER, PIN_SCK, MSBFIRST, drive);
  digitalWrite(PIN_LATCH, HIGH);
  digitalWrite(PIN_LATCH, LOW);
}

// Read the 74HC165. Q7 already holds D7 after the load pulse, so read first, then clock.
// (Arduino shiftIn clocks before reading and would lose the first bit.)
// The shared clock also shifts the 595s, but their outputs only change on LATCH.
uint8_t readSense() {
  digitalWrite(PIN_RX_LOAD, LOW);
  delayMicroseconds(5);
  digitalWrite(PIN_RX_LOAD, HIGH);
  uint8_t v = 0;
  for (int8_t b = 7; b >= 0; b--) {
    if (digitalRead(PIN_RX_DATA)) v |= (uint8_t)(1u << b);
    digitalWrite(PIN_SCK, HIGH);
    digitalWrite(PIN_SCK, LOW);
  }
  return v;
}

void scanCable() {
  for (uint8_t i = 0; i < MAX_WIRES; i++) {
    writeRegisters(ledByte, (uint8_t)(1u << i));
    delayMicroseconds(SETTLE_US);
    rows[i] = readSense();
  }
  writeRegisters(ledByte, 0);   // leave every wire undriven between scans
}

void setup() {
  pinMode(PIN_BTN, INPUT_PULLUP);
  pinMode(PIN_PASS, OUTPUT);
  pinMode(PIN_FAIL, OUTPUT);
  pinMode(PIN_RX_LOAD, OUTPUT);
  pinMode(PIN_LATCH, OUTPUT);
  pinMode(PIN_SER, OUTPUT);
  pinMode(PIN_RX_DATA, INPUT);
  pinMode(PIN_SCK, OUTPUT);
  digitalWrite(PIN_RX_LOAD, HIGH);
  digitalWrite(PIN_LATCH, LOW);
  digitalWrite(PIN_SCK, LOW);
  debounceInit(btn);
  writeRegisters(0, 0);
  Serial.begin(115200);
}

void printReport(CableVerdict v) {
  static const char *names[] = {"ok", "open", "crossed", "short"};
  Serial.print(F("wires="));
  Serial.print(wires);
  Serial.print(v == CABLE_PASS ? F(" PASS") : v == CABLE_REVERSED ? F(" REVERSED") : F(" FAIL"));
  for (uint8_t i = 0; i < wires; i++) {
    Serial.print(' ');
    Serial.print(i + 1);
    Serial.print(':');
    Serial.print(names[wireStatus(rows, i, wires)]);
  }
  Serial.println();
}

void loop() {
  uint32_t now = millis();
  bool blinkOn = (now / BLINK_MS) % 2 == 0;

  if (debouncePressed(btn, digitalRead(PIN_BTN), now)) {
    wires = nextWireCount(wires);
    showCountUntil = now + SHOW_COUNT_MS;
  }

  if ((int32_t)(showCountUntil - now) > 0) {
    ledByte = wireMask(wires);
    digitalWrite(PIN_PASS, LOW);
    digitalWrite(PIN_FAIL, LOW);
    writeRegisters(ledByte, 0);
    return;
  }

  if (now - lastScan >= SCAN_MS) {
    lastScan = now;
    scanCable();
    if (!nothingConnected(rows, wires)) printReport(cableVerdict(rows, wires));
  }

  if (nothingConnected(rows, wires)) {
    ledByte = 0;
    digitalWrite(PIN_PASS, LOW);
    digitalWrite(PIN_FAIL, LOW);
  } else {
    CableVerdict v = cableVerdict(rows, wires);
    VerdictLeds l = verdictLeds(v, blinkOn);
    ledByte = (v == CABLE_REVERSED) ? 0 : wireLeds(rows, wires, blinkOn);
    digitalWrite(PIN_PASS, l.pass ? HIGH : LOW);
    digitalWrite(PIN_FAIL, l.fail ? HIGH : LOW);
  }
  writeRegisters(ledByte, 0);
}

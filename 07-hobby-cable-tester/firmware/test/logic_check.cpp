// Host-side checks for tester_logic.h. Build and run with any host g++:
//   g++ -std=c++11 logic_check.cpp -I../cable_tester -o logic_check && ./logic_check
#include "tester_logic.h"
#include <stdio.h>
#include <string.h>

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

static void straight(uint8_t rows[MAX_WIRES]) {
  for (uint8_t i = 0; i < MAX_WIRES; i++) rows[i] = (uint8_t)(1u << i);
}

int main() {
  uint8_t r[MAX_WIRES];

  // Good straight 8-wire cable
  straight(r);
  for (uint8_t i = 0; i < 8; i++) CHECK(wireStatus(r, i, 8) == WIRE_OK);
  CHECK(cableVerdict(r, 8) == CABLE_PASS);
  CHECK(wireLeds(r, 8, false) == 0xFF);
  CHECK(!nothingConnected(r, 8));

  // Wire 3 open
  straight(r); r[2] = 0;
  CHECK(wireStatus(r, 2, 8) == WIRE_OPEN);
  CHECK(cableVerdict(r, 8) == CABLE_FAIL);
  CHECK(wireLeds(r, 8, true) == (uint8_t)(0xFF & ~(1u << 2)));

  // Wires 1 and 2 swapped
  straight(r); r[0] = 0x02; r[1] = 0x01;
  CHECK(wireStatus(r, 0, 8) == WIRE_CROSSED && wireStatus(r, 1, 8) == WIRE_CROSSED);
  CHECK(cableVerdict(r, 8) == CABLE_FAIL);
  CHECK((wireLeds(r, 8, false) & 0x03) == 0);   // crossed wires blink: off phase
  CHECK((wireLeds(r, 8, true) & 0x03) == 0x03); // on phase

  // Wires 4 and 5 shorted together: driving either shows both far pins
  straight(r); r[3] = 0x18; r[4] = 0x18;
  CHECK(wireStatus(r, 3, 8) == WIRE_SHORT && wireStatus(r, 4, 8) == WIRE_SHORT);
  CHECK(cableVerdict(r, 8) == CABLE_FAIL);

  // Rollover (mirrored) 8-wire cable
  for (uint8_t i = 0; i < 8; i++) r[i] = (uint8_t)(1u << (7 - i));
  CHECK(cableVerdict(r, 8) == CABLE_REVERSED);
  VerdictLeds v = verdictLeds(CABLE_REVERSED, true);  CHECK(v.pass && !v.fail);
  v = verdictLeds(CABLE_REVERSED, false);             CHECK(!v.pass && !v.fail);
  v = verdictLeds(CABLE_FAIL, false);                 CHECK(!v.pass && v.fail);
  v = verdictLeds(CABLE_PASS, false);                 CHECK(v.pass && !v.fail);

  // 4-wire cable: pins 5..8 unused. Stray bits there must be ignored.
  memset(r, 0, sizeof r);
  for (uint8_t i = 0; i < 4; i++) r[i] = (uint8_t)((1u << i) | 0x80);
  CHECK(cableVerdict(r, 4) == CABLE_PASS);
  CHECK(wireLeds(r, 4, true) == 0x0F);
  // 4-wire rollover
  memset(r, 0, sizeof r); r[0] = 0x08; r[1] = 0x04; r[2] = 0x02; r[3] = 0x01;
  CHECK(cableVerdict(r, 4) == CABLE_REVERSED);

  // A single wire is never called reversed
  memset(r, 0, sizeof r); r[0] = 0x01;
  CHECK(cableVerdict(r, 1) == CABLE_PASS);

  // Nothing plugged in
  memset(r, 0, sizeof r);
  CHECK(nothingConnected(r, 8));
  CHECK(cableVerdict(r, 8) == CABLE_FAIL);

  // Helpers
  CHECK(wireMask(8) == 0xFF && wireMask(3) == 0x07);
  CHECK(bitCount(0xA5) == 4);
  CHECK(nextWireCount(2) == 3 && nextWireCount(8) == 2 && nextWireCount(0) == 2);

  // Debounce: bounce for 10 ms, then held low
  Debounce d; debounceInit(d);
  int presses = 0;
  bool levels[] = {false, true, false, true, false};
  for (int t = 0; t < 5; t++) presses += debouncePressed(d, levels[t], (uint32_t)t * 2);
  for (uint32_t t = 10; t < 100; t++) presses += debouncePressed(d, false, t);
  CHECK(presses == 1);
  for (uint32_t t = 100; t < 200; t++) presses += debouncePressed(d, true, t);   // release
  CHECK(presses == 1);

  if (failures == 0) printf("all logic checks passed\n");
  return failures ? 1 : 0;
}

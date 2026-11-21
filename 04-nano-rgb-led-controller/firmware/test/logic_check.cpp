// Host-side checks for lamp_logic.h. Build and run with any host g++:
//   g++ -std=c++11 logic_check.cpp -I../rgb_lamp -o logic_check && ./logic_check
#include "lamp_logic.h"
#include <stdio.h>

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

int main() {
  Rgb red = hsvToRgb(0, 255, 255);   CHECK(red.r == 255 && red.g == 0 && red.b == 0);
  Rgb grn = hsvToRgb(120, 255, 255); CHECK(grn.r == 0 && grn.g == 255 && grn.b == 0);
  Rgb blu = hsvToRgb(240, 255, 255); CHECK(blu.r == 0 && blu.g == 0 && blu.b == 255);
  Rgb wht = hsvToRgb(0, 0, 200);     CHECK(wht.r == 200 && wht.g == 200 && wht.b == 200);
  Rgb off = hsvToRgb(77, 255, 0);    CHECK(off.r == 0 && off.g == 0 && off.b == 0);

  CHECK(potToBrightness(0) == 0);
  CHECK(potToBrightness(1023) == 255);
  CHECK(potToBrightness(-5) == 0 && potToBrightness(5000) == 255);
  CHECK(potToBrightness(512) < 128);   // squared curve: half pot is dimmer than half brightness

  CHECK(nextMode(MODE_OFF) == MODE_WARM);
  CHECK(nextMode((LampMode)(MODE_COUNT - 1)) == MODE_OFF);

  CHECK(triangle(0, 1000) == 0);
  CHECK(triangle(500, 1000) == 255);
  CHECK(triangle(1000, 1000) == 0);

  Rgb o = lampColor(MODE_OFF, 123, 255); CHECK(o.r == 0 && o.g == 0 && o.b == 0);

  Debounce d;
  CHECK(d.update(true, 0) == false);      // bounce phase
  CHECK(d.update(true, 40) == true);      // held 40 ms -> press edge
  CHECK(d.update(true, 80) == false);     // still held, no repeat
  CHECK(d.update(false, 100) == false);
  CHECK(d.update(false, 140) == false);   // release does not report

  printf(failures ? "FAILED: %d\n" : "ALL PASS\n", failures);
  return failures;
}

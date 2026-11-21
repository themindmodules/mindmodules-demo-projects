// Compile-time checks of the control logic using static_assert-style constexpr is not
// possible with the mutable structs, so this file is a compile check only.
// To RUN real tests on a PC, build it with any host g++ and call main():
//   g++ -std=c++11 logic_check.cpp -I../plant_monitor -o logic_check && ./logic_check
#include "plant_logic.h"
#include <stdio.h>

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

int main() {
  PlantConfig c = {620, 300, 30, 45, 10000UL, 1800000UL};
  PlantState s; plantInit(s);

  CHECK(rawToPercent(620, 620, 300) == 0);
  CHECK(rawToPercent(300, 620, 300) == 100);
  CHECK(rawToPercent(460, 620, 300) == 50);
  CHECK(rawToPercent(700, 620, 300) == 0);     // clamp low
  CHECK(rawToPercent(100, 620, 300) == 100);   // clamp high

  CHECK(plantStep(s, c, 600, 1000) == true);   // dry -> pump on
  CHECK(plantStep(s, c, 600, 5000) == true);   // still running
  CHECK(plantStep(s, c, 600, 11000) == false); // max run time reached
  CHECK(plantStep(s, c, 600, 12000) == false); // lockout blocks restart
  CHECK(plantStep(s, c, 600, 11000UL + 1800000UL) == true); // lockout over

  PlantState s2; plantInit(s2);
  CHECK(plantStep(s2, c, 600, 0) == true);
  CHECK(plantStep(s2, c, 400, 2000) == false); // 400 -> ~69 %, above off threshold

  printf(failures ? "FAILED: %d\n" : "ALL PASS\n", failures);
  return failures;
}

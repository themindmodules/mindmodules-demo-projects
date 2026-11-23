#!/bin/sh
# Syntax and warning check (needs arm-none-eabi-g++ or any g++). Does not run the tests.
set -e
CXX=${CXX:-arm-none-eabi-g++}
$CXX -std=c++11 -Wall -Wextra -Werror -fsyntax-only -I../cable_tester logic_check.cpp
$CXX -std=c++11 -Wall -Wextra -Werror -fsyntax-only -x c++ -Istubs -I../cable_tester -include Arduino.h ../cable_tester/cable_tester.ino
echo "OK: tester_logic.h, logic_check.cpp and the sketch compile cleanly (tests not executed by this script)"

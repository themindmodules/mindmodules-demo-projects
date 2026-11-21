#!/bin/sh
# Syntax and warning check (needs arm-none-eabi-g++ or any g++). Does not run the tests.
set -e
CXX=${CXX:-arm-none-eabi-g++}
$CXX -std=c++11 -Wall -Wextra -Werror -fsyntax-only -I../rgb_lamp logic_check.cpp
echo "OK: lamp_logic.h compiles cleanly (tests not executed by this script)"

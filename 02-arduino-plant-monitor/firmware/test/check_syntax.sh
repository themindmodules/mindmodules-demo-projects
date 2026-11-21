#!/bin/sh
# Syntax/warning check of the control logic (needs arm-none-eabi-g++ OR any g++).
set -e
CXX=${CXX:-arm-none-eabi-g++}
$CXX -std=c++11 -Wall -Wextra -Werror -fsyntax-only -I../plant_monitor logic_check.cpp
echo "OK: logic compiles cleanly (tests not executed by this script)"

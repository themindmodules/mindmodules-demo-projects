#!/bin/sh
# Compile-check the app layer for Cortex-M4F (needs arm-none-eabi-gcc on PATH).
# This only checks that the code compiles. It does not run the tests in test_motor_ctl.c.
set -e
OUT="./_build"
mkdir -p "$OUT"
FLAGS="-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Wall -Wextra -Werror -Wshadow -std=c11 -Os"
arm-none-eabi-gcc $FLAGS -c ../App/motor_ctl.c -o "$OUT/motor_ctl.o"
arm-none-eabi-gcc $FLAGS -I../App -Istubs -c ../App/app.c -o "$OUT/app.o"
arm-none-eabi-gcc $FLAGS -I../App -c test_motor_ctl.c -o "$OUT/test_motor_ctl.o"
arm-none-eabi-size "$OUT/motor_ctl.o" "$OUT/app.o"
rm -rf "$OUT"
echo "OK: motor_ctl.c, app.c and the test file compile cleanly"

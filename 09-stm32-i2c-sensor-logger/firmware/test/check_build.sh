#!/bin/sh
# Compile-check the app layer for Cortex-M4 (needs arm-none-eabi-gcc on PATH).
# This only checks that the code compiles. It does not run the tests in test_i2c_util.c.
set -e
OUT="${TMPDIR:-.}/i2c_build_check"
mkdir -p "$OUT"
FLAGS="-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Wall -Wextra -Werror -std=c11 -Os"
arm-none-eabi-gcc $FLAGS -c ../App/i2c_util.c -o "$OUT/i2c_util.o"
arm-none-eabi-gcc $FLAGS -I../App -Istubs -c ../App/app.c -o "$OUT/app.o"
arm-none-eabi-gcc $FLAGS -c test_i2c_util.c -o "$OUT/test_i2c_util.o"
arm-none-eabi-gcc $FLAGS -DAPP_SENSOR_TMP102 -I../App -Istubs -c ../App/app.c -o "$OUT/app_tmp102.o"
rm -rf "$OUT"
echo "OK: i2c_util.c, app.c (LM75 and TMP102 builds) and the test file compile cleanly"

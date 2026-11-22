#!/bin/sh
# Compile-check the app layer for Cortex-M4F (needs arm-none-eabi-gcc on PATH).
# Object files go to a local folder that is removed at the end.
set -e
OUT=./.obj
mkdir -p "$OUT"
trap 'rm -rf "$OUT"' EXIT
FLAGS="-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Wall -Wextra -Werror -Wconversion -std=c11 -Os"
arm-none-eabi-gcc $FLAGS -c ../App/lp_math.c -o "$OUT/lp_math.o"
arm-none-eabi-gcc $FLAGS -I../App -Istubs -c ../App/app.c -o "$OUT/app.o"
arm-none-eabi-size "$OUT/lp_math.o" "$OUT/app.o"
echo "OK: lp_math.c and app.c compile cleanly"

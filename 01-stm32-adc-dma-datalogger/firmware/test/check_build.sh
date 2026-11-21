#!/bin/sh
# Compile-check the app layer for Cortex-M4 (needs arm-none-eabi-gcc on PATH).
set -e
FLAGS="-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Wall -Wextra -Werror -std=c11 -Os"
arm-none-eabi-gcc $FLAGS -c ../App/app_math.c -o /tmp/app_math.o
arm-none-eabi-gcc $FLAGS -I../App -Istubs -c ../App/app.c -o /tmp/app.o
echo "OK: app_math.c and app.c compile cleanly"

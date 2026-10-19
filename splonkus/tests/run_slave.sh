#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/.."
test_binary=$(mktemp /tmp/splonkus-slave.XXXXXX)
trap 'rm -f "$test_binary"' EXIT
"${CC:-clang}" -std=c2x -Wall -Wextra -Werror \
    -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable \
    -Wno-gnu-flexible-array-initializer -ffunction-sections -fdata-sections \
    -DDEBUG -DSPLONKUS -DBOARD_splonkus -DPINOUT_splonkus \
    -DMCU_atmega32u4 -DF_CPU=16000000UL \
    -I../splonk/tests/twi_stubs -I../splonk/tests/avr_stubs \
    -I../lib/include -include lib/lib.h \
    tests/slave_test.c ../lib/src/grb.c -Wl,--gc-sections -o "$test_binary"
"$test_binary"

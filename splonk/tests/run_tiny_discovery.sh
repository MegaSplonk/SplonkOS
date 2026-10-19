#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/.."
test_binary=$(mktemp /tmp/splonk-tiny-discovery.XXXXXX)
trap 'rm -f "$test_binary"' EXIT
"${CC:-clang}" -std=c2x -Wall -Wextra -Werror \
    -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable \
    -Wno-gnu-flexible-array-initializer -ffunction-sections -fdata-sections \
    -DDEBUG -DSPLONKO -DBOARD_splonko -DPINOUT_splonko -DMCU_attiny1614 -DF_CPU=20000000UL \
    -Itests/tiny_stubs -Itests/twi_stubs -Itests/avr_stubs -I../lib/include -include lib/lib.h \
    tests/tiny_discovery_test.c ../lib/src/grb.c -Wl,--gc-sections -o "$test_binary"
"$test_binary"

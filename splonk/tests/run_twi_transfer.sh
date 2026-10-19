#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/.."
test_binary=$(mktemp /tmp/splonk-twi-transfer.XXXXXX)
trap 'rm -f "$test_binary"' EXIT
"${CC:-clang}" -std=c2x -Wall -Wextra -Werror \
    -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable \
    -Wno-gnu-flexible-array-initializer -ffunction-sections -fdata-sections \
    -DDEBUG -DSPLONK -DBOARD_splonk -DPINOUT_splonk -DMCU_atmega32u4 -DF_CPU=16000000UL \
    -Itests/twi_stubs -Itests/avr_stubs -Iinclude -I../lib/include -include lib/lib.h \
    tests/twi_transfer_test.c src/input_packets.c src/input.c src/topology.c \
    src/layers.c src/keymap.c src/report.c src/layout.c src/rgb_control.c ../lib/src/grb.c -Wl,--gc-sections -o "$test_binary"
"$test_binary" left
"$test_binary" right

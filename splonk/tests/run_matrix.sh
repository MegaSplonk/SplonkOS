#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/.."
test_binary=$(mktemp /tmp/splonk-matrix.XXXXXX)
trap 'rm -f "$test_binary"' EXIT
"${CC:-clang}" -std=c2x -Wall -Wextra -Werror \
    -DDEBUG -DSPLONK -DBOARD_splonk -DPINOUT_splonk -DMCU_atmega32u4 -DF_CPU=16000000UL \
    -Itests/avr_stubs -Iinclude -I../lib/include -include lib/lib.h \
    tests/matrix_test.c -o "$test_binary"
"$test_binary"

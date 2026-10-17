#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/.."
test_binary=$(mktemp /tmp/splonk-input-test.XXXXXX)
trap 'rm -f "$test_binary"' EXIT
"${CC:-clang}" -std=c2x -Wall -Wextra -Werror -DBOARD_splonk -DPINOUT_splonk \
    -Itests/avr_stubs -Iinclude -I../lib/include -include lib/lib.h \
    tests/input_test.c src/input.c src/input_packets.c ../lib/src/grb.c -o "$test_binary"
"$test_binary"

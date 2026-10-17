#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/.."
test_binary=$(mktemp /tmp/splonk-shared-bus-test.XXXXXX)
trap 'rm -f "$test_binary"' EXIT
for config in uart full; do
    extra=()
    if [ "$config" = full ]; then extra=(-DSPLONK_FULL_BUS_PINS); fi
    "${CC:-clang}" -std=c2x -Wall -Wextra -Werror \
        -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable \
        -Wno-gnu-flexible-array-initializer -ffunction-sections -fdata-sections \
        -DDEBUG -DSPLONK -DBOARD_splonk -DPINOUT_splonk "${extra[@]}" \
        -Itests/avr_stubs -I../lib/include -include lib/lib.h \
        tests/shared_bus_test.c ../lib/src/grb.c -Wl,--gc-sections -o "$test_binary"
    "$test_binary" left
    "$test_binary" right
done

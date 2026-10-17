#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/.."
test_binary=$(mktemp /tmp/splonk-i2c-test.XXXXXX)
trap 'rm -f "$test_binary"' EXIT
# No MCU selected: hardware-specific transfer bodies are excluded. GC removes
# unused hardware entry points; allocator/registration/removal and grb are real.
"${CC:-clang}" -std=c2x -Wall -Wextra -Werror \
    -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable \
    -Wno-gnu-flexible-array-initializer -ffunction-sections -fdata-sections \
    -DDEBUG -DSPLONK -DBOARD_splonk -DPINOUT_splonk \
    -Itests/avr_stubs -I../lib/include -include lib/lib.h \
    tests/i2c_lifecycle_test.c ../lib/src/grb.c -Wl,--gc-sections -o "$test_binary"
"$test_binary" left
"$test_binary" right

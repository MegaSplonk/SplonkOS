#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/.."
test_binary=$(mktemp /tmp/splonk-layers-test.XXXXXX)
trap 'rm -f "$test_binary"' EXIT
"${CC:-clang}" -std=c2x -Wall -Wextra -Werror \
    -Itests/avr_stubs -Iinclude -I../lib/include -include lib/lib.h \
    tests/layers_test.c src/layers.c -o "$test_binary"
"$test_binary"

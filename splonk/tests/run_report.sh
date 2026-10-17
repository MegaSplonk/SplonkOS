#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/.."
test_binary=$(mktemp /tmp/splonk-report-test.XXXXXX)
trap 'rm -f "$test_binary"' EXIT
"${CC:-clang}" -DBOARD_splonk -std=c2x -Wall -Wextra -Werror \
    -Itests/avr_stubs -Iinclude -I../lib/include -include lib/lib.h \
    tests/report_test.c src/report.c src/layout.c src/layers.c src/keymap.c -o "$test_binary"
"$test_binary"

#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/.."
test_binary=$(mktemp /tmp/splonk-hid-test.XXXXXX)
trap 'rm -f "$test_binary"' EXIT
"${CC:-clang}" -std=c2x -Wall -Wextra -Werror -DSPLONKLIB_ENABLE_USB \
    -DEPTYPE1=7 -DEPTYPE0=6 -DEPDIR=0 \
    -Itests/avr_stubs -I../lib/include -include lib/lib.h \
    tests/hid_transport_test.c ../lib/src/usb/hid/hid.c ../lib/src/usb/hid/descriptors.c -o "$test_binary"
"$test_binary"

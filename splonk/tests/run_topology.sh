#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/.."
test_binary=$(mktemp /tmp/splonk-topology-test.XXXXXX)
trap 'rm -f "$test_binary"' EXIT
"${CC:-clang}" -std=c2x -Wall -Wextra -Werror -Itests/stubs -Iinclude \
    tests/topology_test.c src/topology.c -o "$test_binary"
"$test_binary"

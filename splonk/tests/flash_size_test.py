#!/usr/bin/env python3
"""Exercise the real flash recipe with fake device commands; no hardware access."""
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile

BASE = Path(__file__).resolve().parents[2] / 'Base.mk'
MAKE = shutil.which('make')
SIZE = shutil.which('avr-size')
assert MAKE and SIZE, 'make and avr-size are required'


def record(address, kind, payload):
    data = bytes([len(payload), address >> 8, address & 255, kind]) + payload
    return ':' + (data + bytes([-sum(data) & 255])).hex().upper() + '\n'


def hex_image(ranges):
    result = ''
    upper = None
    for start, length in ranges:
        for address in range(start, start + length, 16):
            next_upper = address >> 16
            if upper != next_upper:
                result += record(0, 4, next_upper.to_bytes(2, 'big'))
                upper = next_upper
            size = min(16, start + length - address)
            result += record(address & 65535, 0, bytes([0x42]) * size)
    return result + record(0, 1, b'')


def executable(path, body):
    path.write_text('#!/bin/sh\n' + body + '\n')
    path.chmod(0o755)


with tempfile.TemporaryDirectory(prefix='splonk-flash-check-') as directory:
    root = Path(directory)
    tools = root / 'tools'
    tools.mkdir()
    trace = root / 'device-actions'
    for command in ('avrdude', 'stty', 'sleep'):
        executable(tools / command,
                   'printf "%s\\n" ' + shlex.quote(command) + ' >> "$FLASH_TEST_LOG"')
    (root / 'empty.mk').write_text('SRC_FILES =\nSRC_FILES_CPP =\nSRC_FILES_CXX =\n')
    (root / 'Makefile').write_text(f'''PROJECT_MAKEFILE := $(abspath $(lastword $(MAKEFILE_LIST)))
PROJECT_NAME = fixture
PROJECT_FILELIST = $(CURDIR)/empty.mk
USE_LIB = 0
PORT = /must-not-access-hardware
BAUD_RATE = 57600
PROGRAMMER = avr109
MCU = atmega32u4
BOARD = splonk
OBJ_DIR_BASE = $(CURDIR)/build
TARGET = firmware
include {BASE}
# Replace device discovery too, so a successful test only records intent.
wait-device:
\t@printf 'wait-device\\n' >> "$$FLASH_TEST_LOG"
''')
    image_dir = root / 'build' / 'fixture'
    image_dir.mkdir(parents=True)
    elf = image_dir / 'firmware.elf'
    image = image_dir / 'firmware.hex'
    # An intentionally unreadable ELF proves that the flashed HEX is checked.
    elf.write_bytes(b'not an ELF; size must come from the HEX image')
    os.utime(elf, (1_000_000_000, 1_000_000_000))
    env = dict(os.environ, PATH=str(tools) + os.pathsep + os.environ['PATH'],
               FLASH_TEST_LOG=str(trace))
    env.pop('MAKEFLAGS', None)
    env.pop('MFLAGS', None)
    checks = 0

    def run(name, contents, *, success=False, args=(), target='flash', size=SIZE,
            expected=''):
        global checks
        if trace.exists():
            trace.unlink()
        image.write_text(contents)
        result = subprocess.run(
            [MAKE, '--no-print-directory', '-j2', target, f'SIZE={size}', *args],
            cwd=root, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            timeout=10)
        actions = trace.read_text().splitlines() if trace.exists() else []
        assert (result.returncode == 0) == success, (name, result.stdout)
        assert expected in result.stdout, (name, result.stdout)
        if success and target == 'flash':
            assert 'wait-device' in actions and 'avrdude' in actions, (name, actions)
            if 'MCU=attiny1614' not in args and 'MCU=atmega328p' not in args:
                assert 'stty' in actions, (name, actions)
        else:
            assert not actions, (name, actions, result.stdout)
        checks += 1
        print(f'PASS {name}')

    for mcu, limit in (('atmega32u4', 28000), ('atmega328p', 28000), ('attiny1614', 15000)):
        run(f'{mcu}: exactly at limit', hex_image([(0, limit)]),
            success=True, args=(f'MCU={mcu}',), expected=f'{limit} / {limit}')
        run(f'{mcu}: one byte over limit', hex_image([(0, limit + 1)]),
            args=(f'MCU={mcu}',), expected='FLASH TOO BIG')
    run('initialized data included: 27900 text + 200 data',
        hex_image([(0, 27900), (27900, 200)]), expected='FLASH TOO BIG')
    run('address gap within limit', hex_image([(0, 2), (27999, 1)]), success=True)
    run('address gap beyond limit despite tiny payload',
        hex_image([(0, 2), (28000, 1)]), expected='FLASH TOO BIG')
    run('non-flash address in flash image', hex_image([(0x810000, 1)]), expected='FLASH TOO BIG')
    run('unknown MCU rejected', hex_image([(0, 16)]), args=('MCU=unknown',),
        expected='No positive SAFE_FLASH_SIZE')
    for value in ('', '0', '-1', 'oops'):
        run(f'invalid limit {value!r}', hex_image([(0, 16)]), args=(f'SAFE_FLASH_SIZE={value}',))
    run('explicit board limit honored', hex_image([(0, 101)]),
        args=('SAFE_FLASH_SIZE=100',), expected='FLASH TOO BIG')
    run('check-only target does not touch devices', hex_image([(0, 16)]),
        success=True, target='check-flash-size')
    run('missing size tool rejected', hex_image([(0, 16)]),
        size=str(tools / 'missing-size'), expected='Cannot measure')
    fake_size = tools / 'fake-size'
    for label, output, status in (
        ('empty output', '', 0),
        ('malformed output', 'not a size', 0),
        ('invalid section size', '.sec1 bad 0\nTotal 1\n', 0),
        ('inconsistent total', '.sec1 12 0\nTotal 1\n', 0),
        ('failed command despite plausible output', '.sec1 12 0\nTotal 12\n', 1),
    ):
        executable(fake_size, 'printf "%s" ' + shlex.quote(output) + f'\nexit {status}')
        run(label, hex_image([(0, 16)]), size=str(fake_size), expected='Cannot')
    run('empty image rejected', record(0, 1, b''))
    run('invalid HEX checksum rejected', ':010000004200\n:00000001FF\n', expected='Cannot')
    print(f'{checks} flash guard checks passed; all device commands were mocked')

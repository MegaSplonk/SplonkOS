# Splonkus

A leaf module using the Splonk ATmega32U4 (16 MHz), 4×6 key matrix and SPI RGB wiring.
It connects to one Splonk host. It has no runtime USB, downstream modules, peer
scanning or bus-enable GPIO handling. Its upstream connection must expose SDA/SCL
to the host without requiring local enable outputs.

The firmware always acts as an I2C slave. It starts at address `0x60`, answers
`IPT_SPLONKUS` discovery probes and accepts the address assigned by the host.
Reads return three bytes of key levels; writes accept the existing one-byte
RGB mode command. Reads before the first scan return released keys. Repeated or
short reads cannot shift the next packet. RGB defaults to key-press lighting.
The existing Splonk host maps this device to its Splonkus/numpad keymap.

Build:

```sh
make -C splonkus hex check-flash-size
```

Output: `build/splonkus/splonkus.hex`.

For a board retaining an AVR109 USB bootloader, enter it using the physical reset
button, then run `make -C splonkus flash PORT=/dev/ttyACM0` with the bootloader's
actual port. The bootloader is separate from this firmware: the application does
not enumerate over USB and cannot respond to a 1200-baud reset request. The target
therefore disables the automatic USB serial reset step. An external programmer
can instead be selected through the usual Makefile overrides.

Run the host-side transport regression:

```sh
bash splonkus/tests/run_slave.sh
```

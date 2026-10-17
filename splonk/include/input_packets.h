#pragma once
#include "lib/pinout.h" // Establish bus-enable pins before the I2C header defaults.
#include "lib/splonk.h"

// Transport adapter only. Zero means the module wire ABI is not defined yet.
usize input_packet_size(ipt type);
// Consumes exactly one complete frame; partial/invalid reads leave the ring
// untouched. No decoder callbacks or topology work execute under this lock.
bool input_packet_read(grb *buffer, void *packet, usize size);

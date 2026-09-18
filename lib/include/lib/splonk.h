#pragma once

#include "lib/proto/i2c.h"

I2C_DATA(splonk, u8 keys[3]; u8 led_mode; ipi peers[4];)

I2C_DATA(splonknob, i8 knob_l; bool butn_l; i8 knob_r; bool butn_r;)

I2C_DATA(dummy, u8 junk;)


void splonk_loop();


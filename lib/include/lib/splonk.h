#pragma once

#include "lib/config.h"
#include "lib/proto/i2c.h"

#if defined(SPLONKLIB_ENABLE_I2C)

I2C_DATA(splonk, u8 keys[3]; u8 led_mode; ipi peers[4];)

I2C_DATA(splonko, u8 keys[1];)

I2C_DATA(
	splonknob, union {
		u8 raw;
		struct {
			u8 l1 : 1;
			u8 b1 : 1;
			u8 r1 : 1;
			u8 l2 : 1;
			u8 b2 : 1;
			u8 r2 : 1;
			u8 _pad : 2;
		};
	};)

I2C_DATA(dummy, u8 junk;)

I2C_DATA(rgb_control, u8 led_mode;)

void splonk_loop();

#endif

#pragma once

#include "lib/config.h"
#include "lib/proto/i2c.h"

#if defined(SPLONKLIB_ENABLE_I2C)

I2C_DATA(splonk, u8 keys[3]; u8 led_mode; ipi peers[4]; u8 discovery;)

// Compatible with feat/splonko: keys only upstream, RGB command downstream.
I2C_DATA(splonko, u8 keys[1];)
I2C_DATA(rgb_control, u8 led_mode;)
// Splonkus has neither USB nor downstream I2C ports: no peer advertisement.
I2C_DATA(splonkus, u8 keys[3];)

// Direction bits carry one step per packet, buttons carry persistent levels.
// Repeated identical direction packets are distinct events, not retransmits.
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

I2C_DATA(
	splonkpad, union {
		u8 raw;
		struct {
			u8 stap : 1;
			u8 dtap : 1;
			u8 ttap : 1;
			u8 _pad : 5;
		};
	};)

I2C_DATA(dummy, u8 junk;)

void splonk_loop();

#endif

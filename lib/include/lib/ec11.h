#pragma once

#ifdef SPLONKLIB_ENABLE_EC11

#include "lib/pin.h"
#include <stdint.h>

struct ec11 {
	pin a, b, btn;
	struct ec11_state {
		bool	button : 1;
		uint8_t rot : 2;
		uint8_t _pad : 5;
	} state;
	int8_t acc;
};

typedef enum ec11_rot : uint8_t {
	ROT_NONE = 0b00, // 0
	ROT_A = 0b01,	 // +1
	ROT_B = 0b11,	 // -1
} ec11_rot;

void ec11_init(struct ec11 *self, pin a, pin b, pin btn);

// Will return the recorded rotation data, and write into `*button` the state of
// the button (can be nullable)
enum ec11_rot ec11_poll(struct ec11 *self, bool *button);

#endif

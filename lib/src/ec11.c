#ifdef SPLONKLIB_ENABLE_EC11

#include "lib/ec11.h"
#include <avr/io.h>

// the macro is here if we ever want to pack those 4 value into a single byte
// each value is at most 2 bit (3 state: 0 and -/+1)
#define R(X, Y, Z, W) X, Y, Z, W

static const enum ec11_rot transition_table[16] = {
	/* old<<2 | new */
	R(ROT_NONE, ROT_B, ROT_A, ROT_NONE), //
	R(ROT_A, ROT_NONE, ROT_NONE, ROT_B), //
	R(ROT_B, ROT_NONE, ROT_NONE, ROT_A), //
	R(ROT_NONE, ROT_A, ROT_B, ROT_NONE), //
};

// read everything from a single function
static struct ec11_state read_state(struct ec11 *self) {
	return (struct ec11_state){.button = pin_get(self->btn),
							   .rot = (pin_get(self->a) ? _BV(1) : 0) |
									  (pin_get(self->b) ? _BV(0) : 0)};
}

static inline int8_t delta_to_int8(enum ec11_rot rot) {
	switch (rot) {
	case (ROT_A):
		return +1;
	case (ROT_B):
		return -1;
	default:
		return 0;
	}
}

void ec11_init(struct ec11 *self, pin a, pin b, pin btn) {
	if (self == nullptr)
		return;

	pin_input(a);
	pin_input(b);
	pin_input(btn);

	self->a = a;
	self->b = b;
	self->btn = btn;
	self->state = read_state(self);
	self->acc = 0;
}

enum ec11_rot ec11_poll(struct ec11 *self, bool *button) {
	struct ec11_state now = read_state(self);
	enum ec11_rot	  delta = transition_table[(self->state.rot << 2) | (now.rot)];

	self->state = now;
	if (button != nullptr)
		*button = self->state.button;

	if (delta == ROT_NONE)
		return ROT_NONE;

	self->acc += delta_to_int8(delta);

	if (self->acc >= 4) {
		self->acc = 0;
		return ROT_A;
	}

	if (self->acc <= -4) {
		self->acc = 0;
		return ROT_B;
	}

	return ROT_NONE;
}

#endif

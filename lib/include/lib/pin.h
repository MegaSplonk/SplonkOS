#pragma once

#include "lib/lib.h"
/*
This file will define a command interface to work with pins.
*/

enum e_port : u8 {
	PORT_A,
	PORT_B,
	PORT_C,
	PORT_D,
	PORT_E,
	PORT_F,
	_PORT_LAST = PORT_F,
};

enum e_pinnum : u8 {
	PIN_0,
	PIN_1,
	PIN_2,
	PIN_3,
	PIN_4,
	PIN_5,
	PIN_6,
	PIN_7,
	_PIN_LAST = PIN_7,
};

struct pin {
	enum e_port	  port : 3;
	enum e_pinnum pin : 3;
	unsigned char _pad : 2;
};

typedef struct pin pin;

extern u8_reg const PORTS_IN[_PORT_LAST + 1];
extern u8_reg const PORTS_OUT[_PORT_LAST + 1];
extern u8_reg const PORTS_DIR[_PORT_LAST + 1];

static inline bool pin_get(pin p) {
	u8_reg port = nullptr;

	port = PORTS_IN[p.port];
	if (port == nullptr)
		return false;
	return (*port & _BV(p.pin));
}

static inline void pin_set(pin p, bool high) {
	u8_reg port = nullptr;

	port = PORTS_OUT[p.port];
	if (port == nullptr)
		return;
	if (high)
		*port |= _BV(p.pin);
	else
		*port &= ~(uint8_t)_BV(p.pin);
}

static inline void pin_toggle(pin p) {
	u8_reg port = nullptr;

	port = PORTS_OUT[p.port];
	if (port == nullptr)
		return;
	*port ^= _BV(p.pin);
}

static inline void pin_output(pin p) {
	u8_reg port = nullptr;

	port = PORTS_DIR[p.port];
	if (port == nullptr)
		return;
	*port |= _BV(p.pin);
}

static inline void pin_input(pin p) {
	u8_reg port = nullptr;

	port = PORTS_DIR[p.port];
	if (port == nullptr)
		return;
	*port &= ~(uint8_t)_BV(p.pin);
}

static_assert(_PORT_LAST <= 0b1111,
			  "enum e_port must only have members up to 7");
static_assert(_PIN_LAST <= 0b111,
			  "enum e_pinnum must only have members up to 7");
static_assert(sizeof(struct pin) == sizeof(uint8_t),
			  "struct pin size must be the same as a uint8_t");

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

bool pin_get(pin p);
void pin_set(pin p, bool high);
void pin_toggle(pin p);
void pin_output(pin p);
void pin_input(pin p);

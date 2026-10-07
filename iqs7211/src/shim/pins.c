#include "lib/pin.h"
#include "shim/Arduino.h"
#include "shim/ArduinoPin.h"

#define P(_PORT, _PIN) {.port = PORT_##_PORT, .pin = PIN_##_PIN, ._pad = 0}

const pin PIN_NB_TO_PIN[] = {
	P(D, 2), P(D, 3), P(D, 1), P(D, 0), P(D, 4), P(C, 6), P(D, 7), P(E, 6),
	P(B, 4), P(B, 5), P(B, 6), P(B, 7), P(D, 6), P(C, 7), P(B, 3), P(B, 1),
	P(B, 2), P(B, 0), P(F, 7), P(F, 6), P(F, 5), P(F, 4), P(F, 1), P(F, 0),
	P(D, 5), P(D, 7), P(B, 4), P(B, 5), P(B, 6), P(D, 6), P(D, 5),
};

t_error pinMode(uint8_t p, uint8_t mode) {
	if (p >= tabsize(PIN_NB_TO_PIN))
		return ERROR;

	if (mode > INPUT_PULLUP)
		return ERROR;

	pin pin_ = PIN_NB_TO_PIN[p];

	// could be pullup
	if (mode & INPUT) {
		pin_input(pin_);
		if (mode == INPUT_PULLUP) {
			if (PORTS_OUT[pin_.port]) {
				*PORTS_OUT[pin_.port] |= 1 << pin_.pin;
			}
		}
	} else {
		pin_output(pin_);
	}

	return SUCCESS;
}

t_error digitalWrite(uint8_t pin, uint8_t state) {
	if (pin >= tabsize(PIN_NB_TO_PIN))
		return ERROR;

	pin_set(PIN_NB_TO_PIN[pin], state == HIGH);
	return SUCCESS;
}

uint8_t digitalRead(uint8_t pin) {
	if (pin >= tabsize(PIN_NB_TO_PIN))
		return 0;

	return pin_get(PIN_NB_TO_PIN[pin]);
}

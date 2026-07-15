#include "lib/pin.h"
#include "lib/config.h"

#include "lib/pin.h"
#include <avr/io.h>

u8_reg const PORTS_IN[_PORT_LAST + 1] = {
#ifdef PINA
	&PINA,
#else
	nullptr,
#endif

#ifdef PINB
	&PINB,
#else
	nullptr,
#endif

#ifdef PINC
	&PINC,
#else
	nullptr,
#endif

#ifdef PIND
	&PIND,
#else
	nullptr,
#endif

#ifdef PINE
	&PINE,
#else
	nullptr,
#endif

#ifdef PINF
	&PINF,
#else
	nullptr,
#endif
};

u8_reg const PORTS_OUT[_PORT_LAST + 1] = {
#ifdef PORTA
	&PORTA,
#else
	nullptr,
#endif

#ifdef PORTB
	&PORTB,
#else
	nullptr,
#endif

#ifdef PORTC
	&PORTC,
#else
	nullptr,
#endif

#ifdef PORTD
	&PORTD,
#else
	nullptr,
#endif

#ifdef PORTE
	&PORTE,
#else
	nullptr,
#endif

#ifdef PORTF
	&PORTF,
#else
	nullptr,
#endif
};

u8_reg const PORTS_DIR[_PORT_LAST + 1] = {
#ifdef DDRA
	&DDRA,
#else
	nullptr,
#endif

#ifdef DDRB
	&DDRB,
#else
	nullptr,
#endif

#ifdef DDRC
	&DDRC,
#else
	nullptr,
#endif

#ifdef DDRD
	&DDRD,
#else
	nullptr,
#endif

#ifdef DDRE
	&DDRE,
#else
	nullptr,
#endif

#ifdef DDRF
	&DDRF,
#else
	nullptr,
#endif
};

bool pin_get(pin p) {
	u8_reg port = nullptr;

	port = PORTS_IN[p.port];
	if (port == nullptr)
		return false;
	return (*port & _BV(p.pin));
}

void pin_set(pin p, bool high) {
	u8_reg port = nullptr;

	port = PORTS_OUT[p.port];
	if (port == nullptr)
		return;
	if (high)
		*port |= _BV(p.pin);
	else
		*port &= ~(uint8_t)_BV(p.pin);
}

void pin_toggle(pin p) {
	u8_reg port = nullptr;

	port = PORTS_OUT[p.port];
	if (port == nullptr)
		return;
	*port ^= _BV(p.pin);
}

void pin_output(pin p) {
	u8_reg port = nullptr;

	port = PORTS_DIR[p.port];
	if (port == nullptr)
		return;
	*port |= _BV(p.pin);
}

void pin_input(pin p) {
	u8_reg port = nullptr;

	port = PORTS_DIR[p.port];
	if (port == nullptr)
		return;
	*port &= ~(uint8_t)_BV(p.pin);
}

static_assert(_PORT_LAST <= 0b111, "enum e_port must only have members up to 7");
static_assert(_PIN_LAST <= 0b111, "enum e_pinnum must only have members up to 7");
static_assert(sizeof(struct pin) == sizeof(uint8_t),
			  "struct pin size must be the same as a uint8_t");

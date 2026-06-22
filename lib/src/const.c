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

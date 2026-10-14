#include <avr/interrupt.h>
#include <avr/io.h>

#include "lib/pin.h"
#include "lib/proto/uart.h"
#include "lib/time.h"
#include "lib/usb/api.h"
#include "lib/usb/cdc.h"

#include <util/delay.h>

void setup(void);
void loop(void);

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

int main(void) {
	sei();
	pu_cdc();
	timer_init();
	uart_init();
	usb_device_attach();

	setup();

	for (;;) {
		loop();
	}

	return 0;
}

#define EXTERNAL_NUM_INTERRUPTS 5

#define EXTERNAL_INT_0 0
#define EXTERNAL_INT_1 1
#define EXTERNAL_INT_2 2
#define EXTERNAL_INT_3 3
#define EXTERNAL_INT_4 4

typedef void (*voidFuncPtr)(void);

static void nothing(void) {}

static volatile voidFuncPtr intFunc[EXTERNAL_NUM_INTERRUPTS] = {
	nothing, nothing, nothing, nothing, nothing,
};

void attachInterrupt(u8 interruptNum, void (*userFunc)(), i16 mode);
void detachInterrupt(u8 interruptNum);

void attachInterrupt(u8 interruptNum, void (*userFunc)(void), i16 mode) {
	if (interruptNum < EXTERNAL_NUM_INTERRUPTS) {
		intFunc[interruptNum] = userFunc;

		// Configure the interrupt mode (trigger on low input, any change, rising
		// edge, or falling edge).  The mode constants were chosen to correspond
		// to the configuration bits in the hardware register, so we simply shift
		// the mode into place.

		// Enable the interrupt.

		switch (interruptNum) {
		// I hate doing this, but the register assignment differs between the 1280/2560
		// and the 32U4.  Since avrlib defines registers PCMSK1 and PCMSK2 that aren't
		// even present on the 32U4 this is the only way to distinguish between them.
		case 0:
			EICRA = (EICRA & ~((1 << ISC00) | (1 << ISC01))) | (mode << ISC00);
			EIMSK |= (1 << INT0);
			break;
		case 1:
			EICRA = (EICRA & ~((1 << ISC10) | (1 << ISC11))) | (mode << ISC10);
			EIMSK |= (1 << INT1);
			break;
		case 2:
			EICRA = (EICRA & ~((1 << ISC20) | (1 << ISC21))) | (mode << ISC20);
			EIMSK |= (1 << INT2);
			break;
		case 3:
			EICRA = (EICRA & ~((1 << ISC30) | (1 << ISC31))) | (mode << ISC30);
			EIMSK |= (1 << INT3);
			break;
		case 4:
			EICRB = (EICRB & ~((1 << ISC60) | (1 << ISC61))) | (mode << ISC60);
			EIMSK |= (1 << INT6);
			break;
		}
	}
}

void detachInterrupt(u8 interruptNum) {
	if (interruptNum < EXTERNAL_NUM_INTERRUPTS) {
		// Disable the interrupt.  (We can't assume that interruptNum is equal
		// to the number of the EIMSK bit to clear, as this isn't true on the
		// ATmega8.  There, INT0 is 6 and INT1 is 7.)
		switch (interruptNum) {
		case 0:
			EIMSK &= ~(1 << INT0);
			break;
		case 1:
			EIMSK &= ~(1 << INT1);
			break;
		case 2:
			EIMSK &= ~(1 << INT2);
			break;
		case 3:
			EIMSK &= ~(1 << INT3);
			break;
		case 4:
			EIMSK &= ~(1 << INT6);
			break;
		}

		intFunc[interruptNum] = nothing;
	}
}

#define IMPLEMENT_ISR(vect, interrupt)                                                             \
	ISR(vect) { intFunc[interrupt](); }

IMPLEMENT_ISR(INT0_vect, EXTERNAL_INT_0)
IMPLEMENT_ISR(INT1_vect, EXTERNAL_INT_1)
IMPLEMENT_ISR(INT2_vect, EXTERNAL_INT_2)
IMPLEMENT_ISR(INT3_vect, EXTERNAL_INT_3)
IMPLEMENT_ISR(INT6_vect, EXTERNAL_INT_4)

#include <avr/interrupt.h>
#include <avr/io.h>

#include "usb_api.h"
#include "usb_cdc.h"

void setup(void);
void loop(void);

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

static void init() {
	// this needs to be called before setup() or some functions won't
	// work there
	sei();

	// on the ATmega168, timer 0 is also used for fast hardware pwm
	// (using phase-correct PWM would mean that timer 0 overflowed half as often
	// resulting in different millis() behavior on the ATmega8 and ATmega168)
	sbi(TCCR0A, WGM01);
	sbi(TCCR0A, WGM00);

	// set timer 0 prescale factor to 64
	sbi(TCCR0B, CS01);
	sbi(TCCR0B, CS00);

	// enable timer 0 overflow interrupt
	sbi(TIMSK0, TOIE0);
}

int main(void) {
	pu_cdc();
	usb_device_attach();
	init();

	setup();

	for (;;) {
		loop();
	}

	return 0;
}

#include "lib/pin.h"
#include "lib/pinout.h"
#include "lib/time.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

/*
pin LED_PIN = {PORT_B, PIN_2, 0};

int main(void) {
	timer_init();
	pin_output(LED_PIN);

	timer_t t = timer_reset();

	while (true) {

		if (timer_wait(&t, 1000)) {
			t = timer_reset();
			pin_toggle(LED_PIN);
		}
	};
}
*/

/*
volatile uint16_t ms = 0;

//-------------------------------------------------
// Initialize PB2 as output
//-------------------------------------------------
void gpio_init(void) {
	PORTB.DIRSET = PIN2_bm;
	PORTB.OUTCLR = PIN2_bm;
}

#undef F_CPU
#define F_CPU 3300000UL

//-------------------------------------------------
// Initialize TCB0 for 1 ms interrupts
//-------------------------------------------------
void timer_init(void) {
	// Disable timer
	TCB0.CTRLA = 0;

	// Period = 1 ms
	// TCB clock = F_CPU / 2 = 10 MHz
	// 10,000 counts = 1 ms
	TCB0.CCMP = ((3310UL / 2) * 20) - 1;

	// Periodic Interrupt mode
	TCB0.CTRLB = TCB_CNTMODE_INT_gc;

	// Enable interrupt
	TCB0.INTCTRL = TCB_CAPT_bm;

	// Enable timer
	TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;
}

//-------------------------------------------------
// 1 ms Interrupt
//-------------------------------------------------
ISR(TCB0_INT_vect) {
	TCB0.INTFLAGS = TCB_CAPT_bm;
	PORTB.OUTTGL = PIN2_bm; // Toggle PB2
}

//-------------------------------------------------
int main(void) {
	gpio_init();
	timer_init();

	sei();

	while (1) {
		// Nothing needed here
	}
}*/

pin p = P9;

static void toggle(void) {
	static milis_t ms = 0;
	if (++ms > 100) {
		ms = 0;
		pin_toggle(p);
	}
}

int main(void) {
	timer_init();
	pin_output(p);
	timer_ontick_register(T_ONTICK_SLOT_USER1, toggle);

	while (1) {
	}
}

#include <avr/interrupt.h>
#include <avr/io.h>

#include "lib/proto/uart.h"
#include "lib/time.h"
#include "lib/usb/api.h"
#include "lib/usb/cdc.h"

void setup(void);
void loop(void);

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

int main(void) {
	pu_cdc();
	usb_device_attach();
	timer_init();
	uart_init();
	sei();

	setup();

	for (;;) {
		loop();
	}

	return 0;
}

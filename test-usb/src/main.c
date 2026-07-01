#include "usb_api.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

int main(void) {
	sei();
	USBDevice__attach();

	while (true) {
		_delay_ms(200);
	}
}

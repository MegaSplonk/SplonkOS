#include "usb_api.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

bool isLUFAbootloader();

int main(void) {
	sei();
	USBDevice__attach();
	TX_RX_LED_INIT;

	while (!USBConnected()) {
		TXLED1;
		RXLED1;
		_delay_ms(100);
		TXLED0;
		RXLED0;
	};

	while (true) {
		Serial__write("Hello from USB !\r\n", 19);
		Serial__flush();
		_delay_ms(100);
	}
}

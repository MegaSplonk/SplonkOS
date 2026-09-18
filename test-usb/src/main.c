#include "lib/pin.h"
#include "lib/pinout.h"
#include "pluggable_usb.h"
#include "usb_api.h"
#include "usb_cdc.h"
#include "usb_hid.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#define KEY 0x07

int main(void) {
	sei();
	pu_cdc();
	// pu_hid();
	// pu_hid_add_desc(&nkro_descriptor);
	// pu_hid_add_desc(&mouse_descriptor);
	usb_device_attach();
	TX_RX_LED_INIT;

	while (!USBConnected()) {
		TXLED1;
		RXLED1;
		_delay_ms(100);
		TXLED0;
		RXLED0;
	};

	// struct HID_NKRO_report report = {};

	// pin_input((pin)D10);
	// pullup
	// PORTB |= _BV(PORTB6);

	while (true) {
		/*
		bool pressed = false;
		if (!pin_get((pin)D10)) {
			pressed = true;
			report.lmeta = true;
			report.keys[KEY >> 3] = _BV((KEY & 7));
		} else {
			report.lmeta = false;
			report.keys[KEY >> 3] = 0;
		}*/

		usb_serial_write((const void *)"Hello from USB !\r\n", 19);
		//usb_serial_write((const void *)(pressed ? "P\r" : "X\r"), 2);
		//hid_send_report(1, &report, sizeof(report));
		usb_serial_flush();
		_delay_ms(100);
	}
}

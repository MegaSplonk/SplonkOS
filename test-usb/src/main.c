#include "lib/pin.h"
#include "lib/pinout.h"
#include "lib/pinout/splonk.h"
#include "lib/proto/adc.h"
#include "lib/proto/uart.h"
#include "lib/time.h"
#include "lib/usb/api.h"
#include "lib/usb/cdc.h"
#include "lib/usb/hid.h"
#include "lib/usb/pu.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/delay.h>

#define KEY 0x07

int main(void) {
	sei();
	uart_init();
	// timer_init();
	pu_cdc();
	pu_hid();
	pu_hid_add_desc(&nkro_descriptor);
	pu_hid_add_desc(&mouse_descriptor);
	pu_hid_add_desc(&cc_descriptor);
	pu_hid_add_desc(&slider_descriptor);
	usb_device_attach();
	// TX_RX_LED_INIT;

	// while (!USBConnected()) {
	// 	TXLED1;
	// 	RXLED1;
	// 	_delay_ms(100);
	// 	TXLED0;
	// 	RXLED0;
	// };

	// struct HID_NKRO_report report_nkro = {};
	// struct HID_MOUSE_report report_mouse = {};
	// struct HID_CC_report report_cc = {};
	// struct HID_SLIDER_report report_slider = {};

	pin_input((pin)D10);
	pin_output((pin)ACT_LED);
	// PORTB |= _BV(PORTB6); // pullup

	timer_t timer = timer_reset();

	while (true) {
		bool pressed = false;
		if (!pin_get((pin)D10)) {
			pressed = true;
			// report_nkro.lmeta = true;
			// report_nkro.keys[KEY >> 3] = _BV((KEY & 7));
			// report_mouse.x = 10;
			// report_cc.vol_inc = 1;
		} else {
			// report_mouse.x = 0;
			// report_nkro.lmeta = false;
			// report_nkro.keys[KEY >> 3] = 0;
			// report_cc.vol_inc = 0;
		}
		// u16	  adc_val = adc_read_pin(ADC_ADC13);
		// char  itoa_buf[20] = {};
		// isize ret = snprintf(itoa_buf, sizeof(itoa_buf), "%u\r\n", adc_val);

		// usb_serial_write((const void *)itoa_buf, (usize)ret);
		// report_slider.slider1 = adc_val * 4;
		if (timer_wait(&timer, 500)) {
			timer = timer_reset();
			pin_toggle((pin)ACT_LED);
		}

		// usb_serial_write((const void *)"Hello from USB !\r\n", 19);
		debug_print((const void *)(pressed ? "P\r" : "X\r"));
		// hid_send_report(HID_REPORT_ID_NKRO, &report_nkro, sizeof(report_nkro));
		// hid_send_report(HID_REPORT_ID_MOUSE, &report_mouse, sizeof(report_mouse));
		// hid_send_report(HID_REPORT_ID_CC, &report_cc, sizeof(report_cc));
		// hid_send_report(HID_REPORT_ID_SLIDER, &report_slider, sizeof(report_slider));
		// usb_serial_flush();
		// _delay_ms(100);
	}
}

#include "lib/ec11.h"
#include "lib/pin.h"
#include "lib/pinout.h"
#include "lib/proto/uart.h"
#include <avr/io.h>
#include <util/delay.h>

#define R_BTN D4
#define R_A	  D2
#define R_B	  D3

int main(void) {
	uart_init();

	struct ec11 ec;
	ec11_init(&ec, (pin)R_A, (pin)R_B, (pin)R_BTN);

	bool btn_prev = false;
	while (true) {
		bool btn_cur = false;
		switch (ec11_poll(&ec, &btn_cur)) {
		case (ROT_A):
			uart_sendstring("CW\r\n");
			break;
		case (ROT_B):
			uart_sendstring("CCW\r\n");
			break;
		case (ROT_NONE):
			break;
		}
		if (!btn_prev && btn_cur) {
			uart_sendstring("\r\n");
		}
		btn_prev = btn_cur;
		_delay_ms(1);
	}

	return 0;
}

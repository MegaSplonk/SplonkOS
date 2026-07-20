#include "lib/ec11.h"
#include "lib/pin.h"
#include "lib/pinout.h"
#include "lib/proto/uart.h"
#include <avr/io.h>
#include <util/delay.h>

#if defined(BOARD_promicro)
#define R_BTN D9
#define R_A	  D10
#define R_B	  D16

#define R_A_LED D6
#define R_B_LED D7

#elif defined(BOARD_nano)

#define R_BTN D10
#define R_A	  D11
#define R_B	  D12

#define R_A_LED D3
#define R_B_LED D4

#elif defined(BOARD_attiny)

#define R_BTN P2
#define R_A	  P3
#define R_B	  P4

#define R_A_LED P5
#define R_B_LED P6

#endif

int main(void) {
	// uart_init();

	struct ec11 ec;
	ec11_init(&ec, (pin)R_A, (pin)R_B, (pin)R_BTN);

	enum ec11_rot last_rot = ROT_NONE;
	bool		  btn_prev = false;
	pin_output((pin)R_A_LED);
	pin_output((pin)R_B_LED);
	pin_set((pin)R_A_LED, false);
	pin_set((pin)R_B_LED, false);
	while (true) {
		bool btn_cur = false;
		bool rot_changed = false;
		switch (ec11_poll(&ec, &btn_cur)) {
		case (ROT_A):
			// uart_sendstring("CW\r\n");
			rot_changed = (last_rot != ROT_A);
			last_rot = ROT_A;
			break;
		case (ROT_B):
			// uart_sendstring("CCW\r\n");
			rot_changed = (last_rot != ROT_B);
			last_rot = ROT_B;
			break;
		case (ROT_NONE):
			break;
		}
		if (!btn_prev && btn_cur) {
			rot_changed = (last_rot != ROT_NONE);
			last_rot = ROT_NONE;
			// uart_sendstring("\r\n");
		}

		if (rot_changed) {
			switch (last_rot) {
			case (ROT_A): {
				pin_set((pin)R_A_LED, true);
				pin_set((pin)R_B_LED, false);
				break;
			};
			case (ROT_B): {
				pin_set((pin)R_A_LED, false);
				pin_set((pin)R_B_LED, true);
				break;
			};
			case (ROT_NONE): {
				pin_set((pin)R_A_LED, false);
				pin_set((pin)R_B_LED, false);
				break;
			};
			}
		}
		btn_prev = btn_cur;
		_delay_ms(1);
	}

	return 0;
}

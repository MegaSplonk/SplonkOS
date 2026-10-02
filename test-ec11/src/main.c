#include "lib/ec11.h"
#include "lib/pin.h"
#include "lib/pinout.h"
#include "lib/proto/uart.h"
#include "lib/time.h"
#include <avr/io.h>
#include <util/delay.h>

#if defined(PINOUT_promicro)
#define R_BTN D9
#define R_A	  D10
#define R_B	  D16

#define R_A_LED D6
#define R_B_LED D7

#elif defined(PINOUT_nano)

#define R_BTN D10
#define R_A	  D11
#define R_B	  D12

#define R_A_LED D3
#define R_B_LED D4

#elif defined(PINOUT_attiny)

#define R_BTN P13
#define R_A	  P12
#define R_B	  P11

#endif

int main(void) {
	uart_init();
	timer_init();

	struct ec11 ec;
	ec11_init(&ec, (pin)R_A, (pin)R_B, (pin)R_BTN);

#if defined(R_A_LED) || defined(R_B_LED)
	enum ec11_rot last_rot = ROT_NONE;
#endif
	bool btn_prev = false;
#ifdef R_A_LED
	pin_output((pin)R_A_LED);
	pin_set((pin)R_A_LED, false);
#endif
#ifdef R_B_LED
	pin_output((pin)R_B_LED);
	pin_set((pin)R_B_LED, false);
#endif
	while (true) {
		bool btn_cur = false;
#if defined(R_A_LED) || defined(R_B_LED)
		bool rot_changed = false;
#endif
		switch (ec11_poll(&ec, &btn_cur)) {
		case (ROT_A):
			uart_sendstring("CW\r\n");
#if defined(R_A_LED) || defined(R_B_LED)
			rot_changed = (last_rot != ROT_A);
			last_rot = ROT_A;
#endif
			break;
		case (ROT_B):
			uart_sendstring("CCW\r\n");
#if defined(R_A_LED) || defined(R_B_LED)
			rot_changed = (last_rot != ROT_B);
			last_rot = ROT_B;
#endif
			break;
		case (ROT_NONE):
			break;
		}
		if (!btn_prev && btn_cur) {
#if defined(R_A_LED) || defined(R_B_LED)
			rot_changed = (last_rot != ROT_NONE);
			last_rot = ROT_NONE;
#endif
			uart_sendstring("\r\n");
		}

#if defined(R_A_LED) && defined(R_B_LED)
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
#endif
		btn_prev = btn_cur;
	}

	return 0;
}

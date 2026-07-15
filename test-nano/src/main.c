#include "lib/lib.h"
#include "lib/pin.h"
#include "lib/pinout.h"
#include "lib/proto/adc.h"
#include "lib/proto/uart.h"
#include "lib/time.h"
#include <avr/io.h>
#include <util/delay.h>

static void toggle_my_led(void) {
	static u16 tick = 0;

	if (++tick >= 1000) {
		tick = 0;
		pin_toggle((pin)D2);
	}
}

static void send_message(void) {
	static u16 tick = 0;

	if (++tick >= 1000) {
		tick = 0;
		debug_print("This is a message from a timer");
	};
}

int main(void) {
	timer_init();
	uart_init();

	timer_ontick_register(T_ONTICK_SLOT_USER1, toggle_my_led);
	timer_ontick_register(T_ONTICK_SLOT_USER2, send_message);
	pin_output((pin)D2);

	while (true) {
	}

	return 0;
}

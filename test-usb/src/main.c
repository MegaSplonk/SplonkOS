#include <avr/io.h>
#include <util/delay.h>

#include "lib/lib.h"
#include "lib/pin.h"
#include "lib/pinout.h"
#include "lib/proto/uart.h"

u32 milis = 0;

int main(void) {
	uart_init();

	debug_print("Start");
	u16 i = 0;
	// welcome home fake timers
	// i'll have to make you one day
	// aka next pr
	u32 timer_print = milis + 500;
	while (1) {
		_delay_ms(1);
		milis++;
		if (timer_print < milis) {
			timer_print = milis + 500;
			debug_print("HELLO %hu !!!!", i++);
		}
		uart_poll();
	}
	debug_print("End");
	return 0;
}

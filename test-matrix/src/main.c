#include "lib/pin.h"
#include "lib/pinout.h"
#include "lib/proto/uart.h"
#include "lib/time.h"
#include <avr/io.h>
#include <util/delay.h>

#include "lib/key_matrix.h"
#include "lib/rgb_matrix.h"

#define BOOLSTR(b) ((b) ? '.' : 'X')

#define K(i) BOOLSTR(keys[0] & _BV(i))

int main(void) {
	timer_init();
	uart_init();
	debug_print("%s\n", "I'm splonko");
	rm_init();
	km_init();
	timer_t led_updt_timer = timer_reset();
	timer_t led_mode_timer = timer_reset();
	u8		keys[1] = {};
	rm_set_keys(keys);

	timer_t print_timer = timer_reset();

	while (true) {
		km_scan(keys, 1);

		// static typeof(keys) prev = {};
		// bool				changed = false;

		if (timer_wait(&print_timer, 500)) {
			print_timer = timer_reset();
			debug_print("==");
			debug_print("%c %c %c", K(0), K(1), K(2));
			debug_print("%c %c %c", K(3), K(4), K(5));
			debug_print("%c %c o", K(6), K(7));
			debug_print("==");
			// keys[0] = 0;
		}
		if (timer_wait(&led_updt_timer, 5)) {
			led_updt_timer = timer_reset();
			rm_update();
			// FIXME - keycodes are borked? Nothing can be read from row1
		}
		// need to call km_scan and then rm_set_keys
		if (timer_wait(&led_mode_timer, 5000)) {
			led_mode_timer = timer_reset();
			// led_mode mode;
			// rm_get_mode(&mode);
			rm_set_mode(RM_TOUCH); //(mode + 1) % RM_MODE_COUNT
			debug_print("keys %02x", keys[0]);
		}
	}

	return 0;
}

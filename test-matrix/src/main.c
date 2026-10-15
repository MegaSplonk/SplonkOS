#include "lib/pinout.h"
#include "lib/time.h"
#include "lib/proto/uart.h"
#include <avr/io.h>
#include <util/delay.h>

#include "lib/rgb_matrix.h"

int main(void) {
  timer_init();
	uart_init();
	debug_print("%s\n", "I'm splonko");
	rm_init();
  timer_t led_updt_timer = timer_reset();
  timer_t led_mode_timer = timer_reset();

	while (true) {
    if (timer_wait(&led_updt_timer, 5)) {
      led_updt_timer = timer_reset();
		  rm_update();
    }
    // need to call km_scan and then rm_set_keys
    if (timer_wait(&led_mode_timer, 5000)) {
      led_mode_timer = timer_reset();
      led_mode mode;
      rm_get_mode(&mode);
      rm_set_mode((mode + 1) % RM_MODE_COUNT);
      debug_print("mode %d", mode);
    }
	}

	return 0;
}

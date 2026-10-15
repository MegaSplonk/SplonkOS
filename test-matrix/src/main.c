#include "lib/pinout.h"
#include "lib/time.h"
#include "lib/proto/rgb.h"
#include "lib/proto/uart.h"
#include <avr/io.h>
#include <util/delay.h>

#include "lib/rgb_matrix.h"

int main(void) {
  timer_init();
	uart_init();
	rm_init();

	while (true) {
		rm_update();
    // need to call km_scan and then rm_set_keys
	}

	return 0;
}

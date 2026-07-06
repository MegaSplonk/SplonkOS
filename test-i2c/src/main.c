#include "lib/proto/i2c.h"
#include "lib/proto/uart.h"
#include <avr/io.h>
#include <util/delay.h>

int main(void) {
	uart_init();
	i2c_init();

	while (true) {
		_delay_ms(5000);
		// Need to impl Slave Recv functions for actual tests
	}

	return 0;
}

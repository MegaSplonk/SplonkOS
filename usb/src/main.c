#include <avr/io.h>
#include <util/delay.h>

#include "lib/pin.h"
#include "lib/pinout.h"

int main(void) {
	pin led_pin1 = D10;
	pin led_pin2 = D16;
	pin_output(led_pin1);
	pin_output(led_pin2);
	
	pin_set(led_pin2, true);

	while (true) {
		_delay_ms(1000);
		pin_toggle(led_pin1);
		pin_toggle(led_pin2);
	};
	return 0;
}

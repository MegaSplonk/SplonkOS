#include <avr/io.h>
#include <util/delay.h>

#include "lib/pin.h"
#include "lib/pinout.h"

int main(void) {
	pin		leds[3] = {D16, D10, D14};
	uint8_t phases[] = {
		0b000, 0b100, 0b010, 0b001, 0b011, 0b111, 0b110, 0b100,
	};

	for (uint8_t i = 0; i < tabsize(leds); i++)
		pin_output(leds[i]);
	for (uint8_t i = 0; i < tabsize(leds); i++)
		pin_set(leds[i], false);
	uint8_t p = 0;

	while (true) {
		_delay_ms(500);
		for (uint8_t i = 0; i < tabsize(leds); i++)
			pin_set(leds[i], phases[p] & _BV(i));
		p++;
		p %= tabsize(phases);
	};
	return 0;
}

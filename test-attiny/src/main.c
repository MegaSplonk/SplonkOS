#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>

#define LED_PIN PIN2_bm

int main(void) {
	PORTB.DIRSET = LED_PIN;

	while (true) {
		_delay_ms(1000);
		PORTB.OUTTGL |= LED_PIN;
	};
}

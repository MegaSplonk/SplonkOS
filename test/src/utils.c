#include <stdint.h>

static inline void spin_loop(volatile uint16_t counts) {
	while (counts)
		counts--;
}

void delay_ms(uint16_t ms) {
	while (ms) {
		// this value was taken using a delay of 500ms, and just recording the led
		// blinking. it seems to be high enough such that each loop of delay_loop
		// takes 1ms :D
		spin_loop((F_CPU) / 5000);
		ms--;
	}
}


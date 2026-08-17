#include "lib/pinout.h"
#include "lib/proto/rgb.h"
#include "lib/proto/uart.h"
#include "lib/time.h"
#include <avr/io.h>
#include <util/delay.h>

rgb leds[16] = {};
#define RED	  ((rgb){255, 0, 0})
#define GREEN ((rgb){0, 255, 0})
#define OFF	  ((rgb){0, 0, 0})

static rgb hue_to_rgb(u16 phase) {
	uint8_t h = phase >> 8; // 0..255

	uint8_t sector = h >> 6; // 0..3
	uint8_t x = (h & 63) << 2;

	switch (sector) {
	case 0:
		return (rgb){255, x, 0};

	case 1:
		return (rgb){255 - x, 255, 0};

	case 2:
		return (rgb){0, 255, x};

	default:
		return (rgb){0, 255 - x, 255};
	}
}

int main(void) {
	timer_init();
	uart_init();
	debug_print("HELLO");
	led_init((led_config){
		.buffer = leds,
		.brightness = 4,
		.buffer_len = tabsize(leds),
		.clock = D13,
		.data = D11,
	});

	for (usize i = 0; i < tabsize(leds); i++)
		leds[i] = OFF;
	led_update();
	u16		color = 0;
	timer_t timer = timer_reset();

	while (true) {
		if (timer_wait(&timer, 2)) {
			timer = timer_reset();
			color += 100;
			rgb col = hue_to_rgb(color);
			for (usize i = 0; i < tabsize(leds); i++) {
				leds[i] = col;
			}
			led_update();
		}
	}

	return 0;
}

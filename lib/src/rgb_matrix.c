#include "lib/rgb_matrix.h"
#include "lib/proto/rgb.h"
#include "lib/proto/uart.h"
#include "lib/time.h"

#ifdef RGB_MATRIX

rgb		 leds[rm_size] = {};
led_mode mode;
u8		*keys;

void rm_init(void) {
	led_init((led_config){
		.buffer = leds, .buffer_len = tabsize(leds), .brightness = 2, .clock = SCLK, .data = MOSI});
	rm_set_mode(default_led_mode);
	rm_update();
}

void rm_set_mode(led_mode m) { mode = m; }

void rm_set_keys(u8 *k) { keys = k; }

void rm_get_mode(led_mode *m) { *m = mode; }

#define RED ((rgb){255, 0, 0})
#define OFF ((rgb){0, 0, 0})

static void rgb_matrix_off() {
	for (usize i = 0; i < tabsize(leds); i++)
		leds[i] = OFF;
}

static void rgb_matrix_on() {
	for (usize i = 0; i < tabsize(leds); i++) {
		leds[i] = RED;
	}
}

static void rgb_matrix_rainbow() {
	for (usize i = 0; i < tabsize(leds); i++) {
		leds[i] = RED;
	}
}

static void rgb_matrix_snake() {
	static usize current_idx = 0;
	for (usize i = 0; i < tabsize(leds); i++) {
		if (i == current_idx) {
			leds[i] = RED;
		} else {
			leds[i] = OFF;
		}
	}
	current_idx = (current_idx + 1) % rm_size;
}

static void rgb_matrix_touch() {
  if (!keys)
    return;
	for (usize c = 0; c < rm_cols_size; c++) {
		for (usize r = 0; r < rm_rows_size; r++) {
			usize key = c * rm_cols_size + r;
			if (keys[key / 8] & (1 << (key % 8))) {
				leds[key] = RED;
			} else {
				leds[key] = OFF;
			}
		}
	}
}

// define other funcs

void rm_update(void) {
	typedef void (*rgb_matrix_func)(void);

	const rgb_matrix_func rgb_matrix_mode[] = {
		[RM_OFF] = &rgb_matrix_off,			[RM_ON] = &rgb_matrix_on,
		[RM_RAINBOW] = &rgb_matrix_rainbow, [RM_GD_HOR] = &rgb_matrix_off,
		[RM_GD_VER] = &rgb_matrix_off,		[RM_GD_DIA] = &rgb_matrix_off,
		[RM_SNAKE] = &rgb_matrix_snake,		[RM_TOUCH] = &rgb_matrix_touch,
	};

	static timer_t led_tick_timer = {};

	if (!timer_wait(&led_tick_timer, led_tick_rate)) {
		return;
	}
	led_tick_timer = timer_reset();

	rgb_matrix_mode[mode]();

	debug_print("poopoo %d", mode);

	led_update();
}

#endif

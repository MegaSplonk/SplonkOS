#pragma once

#include "lib/config.h"
#ifdef SPLONKLIB_ENABLE_SPI

#include "lib/proto/spi.h"
#include <avr/io.h>

typedef struct rgb {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} rgb;

void led_set_colors(rgb col[], uint8_t brightness[], uint8_t length);
static inline void led_set_color(rgb col, uint8_t brightness) {
	led_set_colors(&col, &brightness, 1);
}

#endif

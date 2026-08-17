#pragma once

#include "lib/config.h"
#include "lib/pin.h"

#ifdef SPLONKLIB_ENABLE_LED
#include <avr/io.h>

typedef struct rgb {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} rgb;

typedef struct led_config {
	rgb	 *buffer;
	usize buffer_len;
	u8	  brightness;
	pin	  data;
	pin	  clock;
} led_config;

t_error led_init(led_config config);
t_error led_set_brightness(u8 brightness);
t_error led_update(void);

#endif

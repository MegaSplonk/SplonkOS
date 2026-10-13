#include "lib/proto/rgb.h"
#include "lib/config.h"
#include "lib/lib.h"
#include "lib/pin.h"
#include "lib/proto/spi.h"
#include "lib/proto/uart.h"
#include <util/delay.h>

#ifdef SPLONKLIB_ENABLE_LED
#if !defined(SPLONKLIB_LED_BITBANG) && !defined(SPLONKLIB_LED_SPI)
#error "You need to define either SPLONKLIB_LED_BITBANG or SPLONKLIB_LED_SPI"
#endif

#if !defined(SPLONKLIB_ENABLE_SPI) && defined(SPLONKLIB_LED_SPI)
#error "SPLONKLIB_LED_SPI needs SPLONKLIB_ENABLE_SPI"
#endif

#if defined(SPLONKLIB_LED_BITBANG)

#define L_spi_init	fpi_init
#define L_spi_write fpi_write

static void fpi_init(void);
static void fpi_write(u8);

#elif defined(SPLONKLIB_ENABLE_SPI)

#define L_spi_init	spi_init
#define L_spi_write spi_write

#endif

static led_config CONFIG = {};

t_error led_set_brightness(u8 brightness) {
	if (CONFIG.buffer == NULL)
		return ERROR;
	if (brightness > 31)
		brightness = 31;
	CONFIG.brightness = brightness;
	return SUCCESS;
}

t_error led_init(led_config config) {
	if (config.buffer == nullptr)
		return ERROR;
	// cant set the config again
	if (CONFIG.buffer != nullptr)
		return ERROR;
	CONFIG = config;

	L_spi_init();
	return SUCCESS;
}

t_error led_update(void) {
	if (CONFIG.buffer == nullptr)
		return ERROR;

	for (u8 i = 0; i < 4; i++)
		L_spi_write(0x00);
	// led data
	for (usize i = 0; i < CONFIG.buffer_len; i++) {
		L_spi_write(0xE0 + (CONFIG.brightness & 31));
		L_spi_write(CONFIG.buffer[i].b);
		L_spi_write(CONFIG.buffer[i].g);
		L_spi_write(CONFIG.buffer[i].r);
	}

	// reset frame
	for (u8 i = 0; i < 4; i++)
		L_spi_write(0x00);

	// End frame: (buffer_len / 2) bits of zero
	for (u8 i = 0; i < (CONFIG.buffer_len >> 1); i += 8) {
		L_spi_write(0x00);
	}
	return SUCCESS;
}

#if defined(SPLONKLIB_LED_BITBANG)

static void fpi_init(void) {
	pin_output(CONFIG.data);
	pin_output(CONFIG.clock);
	pin_set(CONFIG.clock, false);
}

#define nop() __asm volatile("nop")
static void fpi_write(u8 byte) {
	for (u8 i = 0; i < 8; i++) {
		pin_set(CONFIG.data, !!(byte & 0x80));
		pin_set(CONFIG.clock, false);
		byte <<= 1;
		nop();
		nop();
		pin_set(CONFIG.clock, true);
	}
}

#endif

#endif

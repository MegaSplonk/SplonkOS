#include "lib/proto/rgb.h"
#include "lib/config.h"
#include "lib/lib.h"
#include "lib/proto/spi.h"

#ifdef SPLONKLIB_ENABLE_SPI

void led_set_colors(rgb col[], uint8_t brightness[], uint8_t length) {
	// start frame
	for (uint8_t i = 0; i < 4; i++)
		spi_write(0x00);
	// led data
	for (uint8_t i = 0; i < length; i++) {
		spi_write(0xE0 + (brightness[i] & 31));
		spi_write(col[i].b);
		spi_write(col[i].g);
		spi_write(col[i].r);
	}

	// reset frame
	for (uint8_t i = 0; i < 4; i++)
		spi_write(0x00);

	// // End frame: 8+8*(leds >> 4) clock cycles
	// for (uint8_t i = 0; i < length; i += 16) {
	// 	spi_write(0x00);  // 8 more clock cycles
	// }
}

#endif

#include "lib/proto/spi.h"
#include "lib/pin.h"
#include "lib/pinout.h"
#include "lib/proto/uart.h"

#ifdef SPLONKLIB_ENABLE_SPI

void spi_init(void) {
	pin_output((pin)MOSI);
	pin_output((pin)SCLK);
	pin_output((pin)SS);

	SPCR = _BV(SPE) | _BV(MSTR) | _BV(SPR1) | _BV(SPR0);
}

void spi_uninit(void) { SPCR = 0; }

bool spi_write(uint8_t data) {
	// load data into register
	SPDR = data;

	// Wait for transmission complete
	while (!(SPSR & _BV(SPIF)))
		;

	return false;
}

#endif

#include "lib/proto/spi.h"
#include "lib/pin.h"
#include "lib/pinout.h"
#include "lib/proto/uart.h"

void spi_init(void) {
	pin_output(MOSI);
	pin_output(SCLK);
	pin_output(SS);

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

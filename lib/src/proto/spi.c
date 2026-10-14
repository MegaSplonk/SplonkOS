#include "lib/proto/spi.h"
#include "lib/pin.h"
#include "lib/pinout.h"
#include "lib/proto/uart.h"
#include <avr/io.h>

#ifdef SPLONKLIB_ENABLE_SPI

#if defined(MCU_atmega328p) || defined(MCU_atmega32u4)

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

#elif defined(MCU_attiny1614)

#include <avr/io.h>
#include <stdbool.h>
#include <stdint.h>

void spi_init(void) {
	pin_output((pin)MOSI);
	pin_output((pin)SCLK);
	pin_output((pin)SS);

	// Normal SPI mode (Mode 0), no buffering
	SPI0.CTRLB = SPI_SSD_bm;

	// Enable SPI, master mode, F_CPU / 128
	SPI0.CTRLA = SPI_ENABLE_bm | SPI_MASTER_bm | SPI_PRESC_DIV128_gc;
}

void spi_uninit(void) { SPI0.CTRLA = 0; }

bool spi_write(uint8_t data) {
	// Start transmission
	SPI0.DATA = data;

	// Wait for transmission complete
	while (!(SPI0.INTFLAGS & SPI_IF_bm))
		;

	// Read received data
	uint8_t dummy = SPI0.DATA;
	(void)dummy;

	return false;
}

#endif

#endif

#pragma once

#include "lib/config.h"
#ifdef SPLONKLIB_ENABLE_SPI
#include "lib/pinout.h"

#include <stdint.h>

#if !defined(MOSI) || !defined(MISO) || !defined(SCLK) || !defined(SS)
#error "SPI pinout undefined"
#endif

void spi_init(void);
void spi_uninit(void);
bool spi_write(uint8_t data);
#endif

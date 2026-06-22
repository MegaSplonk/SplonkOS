#pragma once

#ifdef MCU_atmega32u4

#define SPLONKLIB_ENABLE_USB
#define SPLONKLIB_ENABLE_SPI
#define SPLONKLIB_ENABLE_I2C
// the atmega32u4 has uart, but it is weird and not wired to the USB.

#endif

#ifdef MCU_atmega328p

#define SPLONKLIB_ENABLE_UART
#define SPLONKLIB_ENABLE_SPI
#define SPLONKLIB_ENABLE_I2C

#endif

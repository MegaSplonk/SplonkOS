#pragma once

#if defined(MCU_atmega32u4)

#define SPLONKLIB_ENABLE_USB
#define SPLONKLIB_ENABLE_SPI
#define SPLONKLIB_ENABLE_I2C
// the atmega32u4 has uart, but it is weird and not wired to the USB.
#define SPLONKLIB_ENABLE_EC11
#define SPLONKLIB_ENABLE_UART
#define SPLONKLIB_ENABLE_LED
#define SPLONKLIB_LED_SPI

#elif defined(MCU_atmega328p)

#define SPLONKLIB_ENABLE_UART
#define SPLONKLIB_ENABLE_SPI
#define SPLONKLIB_ENABLE_I2C
#define SPLONKLIB_ENABLE_EC11
#define SPLONKLIB_ENABLE_LED

#define SPLONKLIB_LED_BITBANG

#elif defined(MCU_attiny1614)

#define SPLONKLIB_ENABLE_EC11

#endif

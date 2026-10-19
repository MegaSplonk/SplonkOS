#pragma once

// all feature defines
#if false

#define SPLONKLIB_ENABLE_EC11
#define SPLONKLIB_ENABLE_I2C
#define SPLONKLIB_ENABLE_LED
#define SPLONKLIB_ENABLE_SCREEN
#define SPLONKLIB_ENABLE_SPI
#define SPLONKLIB_ENABLE_UART
#define SPLONKLIB_ENABLE_USB
#define SPLONKLIB_ENABLE_ADC

#define SPLONKLIB_DISABLE_USB_LED

#define SPLONKLIB_LED_SPI
#define SPLONKLIB_LED_BITBANG

#endif

#if defined(BOARD_usb_test)

#define SPLONKLIB_ENABLE_UART
#define SPLONKLIB_ENABLE_USB

#elif defined(BOARD_matrix_test)

#define SPLONKLIB_ENABLE_UART
#define SPLONKLIB_ENABLE_LED
#define SPLONKLIB_LED_BITBANG

#elif defined(BOARD_rgb_test)

#define SPLONKLIB_ENABLE_UART
#define SPLONKLIB_ENABLE_LED
#define SPLONKLIB_LED_BITBANG

#elif defined(BOARD_i2c_test)

#define SPLONKLIB_ENABLE_I2C
#define SPLONKLIB_ENABLE_UART
#define SPLONKLIB_ENABLE_LED
#define SPLONKLIB_ENABLE_SPI
#define SPLONKLIB_LED_SPI
#ifdef PINOUT_promicro
#define SPLONKLIB_ENABLE_USB
#endif

#elif defined(BOARD_splonkpad_test)

#define SPLONKLIB_ENABLE_UART
#define SPLONKLIB_ENABLE_I2C
#define SPLONKLIB_ENABLE_USB

#elif defined(BOARD_splonkreen_test)

#define SPLONKLIB_ENABLE_UART
#define SPLONKLIB_ENABLE_SPI
#define SPLONKLIB_ENABLE_SCREEN

#elif defined(BOARD_splonknob_test)

#define SPLONKLIB_ENABLE_UART
#define SPLONKLIB_ENABLE_EC11

#elif defined(BOARD_splonkbar_test)

#define SPLONKLIB_ENABLE_UART
#define SPLONKLIB_ENABLE_ADC
#define SPLONKLIB_ENABLE_I2C

#elif defined(BOARD_splonko_test)

#define SPLONKLIB_ENABLE_UART

#elif defined(BOARD_iqs7211e_test)

#define SPLONKLIB_ENABLE_USB
#define SPLONKLIB_ENABLE_UART

#elif defined(BOARD_splonk)

#define SPLONKLIB_ENABLE_I2C
#define SPLONKLIB_ENABLE_UART
#define SPLONKLIB_ENABLE_LED
#define SPLONKLIB_ENABLE_SPI
#define SPLONKLIB_LED_SPI
#define SPLONKLIB_ENABLE_USB
#ifndef SPLONK_FULL_BUS_PINS
#define SPLONKLIB_UART_FORCE_HARDWARE
#endif

#elif defined(BOARD_splonkus)

#define SPLONKLIB_ENABLE_I2C
#define SPLONKLIB_I2C_SLAVE_ONLY
#define SPLONKLIB_ENABLE_UART
#define SPLONKLIB_ENABLE_LED
#define SPLONKLIB_ENABLE_SPI
#define SPLONKLIB_LED_SPI

#elif defined(BOARD_splonkpad)

#define SPLONKLIB_ENABLE_I2C
#define SPLONKLIB_ENABLE_UART

#elif defined(BOARD_splonkreen)

#define SPLONKLIB_ENABLE_SCREEN
#define SPLONKLIB_ENABLE_UART
#define SPLONKLIB_ENABLE_I2C
#define SPLONKLIB_ENABLE_SPI

#elif defined(BOARD_splonknob)

#define SPLONKLIB_ENABLE_UART
#define SPLONKLIB_ENABLE_EC11
#define SPLONKLIB_ENABLE_I2C

#elif defined(BOARD_splonkbar)

#define SPLONKLIB_ENABLE_UART
#define SPLONKLIB_ENABLE_ADC
#define SPLONKLIB_ENABLE_I2C

#elif defined(BOARD_splonko)

#define SPLONKLIB_ENABLE_I2C
#define SPLONKLIB_ENABLE_UART
#define SPLONKLIB_ENABLE_LED
#define SPLONKLIB_LED_BITBANG

#elif defined(BOARD_i2c_test)

#define SPLONKLIB_ENABLE_UART
#define SPLONKLIB_ENABLE_I2C

#else
#error "No known board has been defined !"
#endif

// UART PART
#if defined(SPLONKLIB_ENABLE_UART)

#if defined(MCU_atmega32u4)

#if defined(SPLONKLIB_ENABLE_USB) && !defined(SPLONKLIB_UART_FORCE_HARDWARE)
#define SPLONKLIB_UART_USB
#else
#define SPLONKLIB_UART_HARDWARE
#endif

#else

#define SPLONKLIB_UART_HARDWARE

#endif

#endif

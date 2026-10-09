#pragma once

#include <avr/io.h>
#include <util/delay.h>

#define delay _delay_ms

#define HIGH 0x01
#define LOW	 0x00

#define INPUT		 0x0
#define OUTPUT		 0x1
#define INPUT_PULLUP 0x2

#define CHANGE	1
#define FALLING 2
#define RISING	3

#ifndef lowByte
#define lowByte(w) ((uint8_t)((w) & 0xff))
#endif

#ifndef highByte
#define highByte(w) ((uint8_t)((w) >> 8))
#endif

#ifdef __cplusplus
extern "C" {
#endif
unsigned long micros(void);
unsigned long millis(void);
#ifdef __cplusplus
}
#endif

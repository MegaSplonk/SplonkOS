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

u32 millis(void);

void attachInterrupt(u8 interruptNum, void (*userFunc)(void), i16 mode);
void detachInterrupt(u8 interruptNum);

#ifdef __cplusplus
}
#endif

#define NOT_AN_INTERRUPT -1
#define digitalPinToInterrupt(p)                                                                   \
	((p) == 0                                                                                      \
		 ? 2                                                                                       \
		 : ((p) == 1 ? 3 : ((p) == 2 ? 1 : ((p) == 3 ? 0 : ((p) == 7 ? 4 : NOT_AN_INTERRUPT)))))

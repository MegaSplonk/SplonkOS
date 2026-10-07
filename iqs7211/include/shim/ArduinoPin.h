#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "lib/lib.h"

t_error pinMode(uint8_t p, uint8_t mode);
t_error digitalWrite(uint8_t pin, uint8_t state);
uint8_t digitalRead(uint8_t pin);
void	attachInterrupt(uint8_t interruptNum, void (*userFunc)(void), int mode);
void	detachInterrupt(uint8_t interruptNum);

#ifdef __cplusplus
}
#endif

static const uint8_t SDA = 2;
static const uint8_t SCL = 3;

#define NOT_AN_INTERRUPT -1
#define digitalPinToInterrupt(p)                                                                   \
	((p) == 0                                                                                      \
		 ? 2                                                                                       \
		 : ((p) == 1 ? 3 : ((p) == 2 ? 1 : ((p) == 3 ? 0 : ((p) == 7 ? 4 : NOT_AN_INTERRUPT)))))

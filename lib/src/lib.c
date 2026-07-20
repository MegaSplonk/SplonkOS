#include "lib/lib.h"
#include <stdint.h>

#include <avr/io.h>

char FMT_BUF[256] = {};

#if defined(MCU_atmega328p) || defined(MCU_atmega32u4)

__attribute__((constructor)) void mcu_init(void) {}

#elif defined(MCU_attiny1614)

__attribute__((constructor)) void mcu_init(void) {
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.MCLKCTRLB = 0;
}

#else

#endif

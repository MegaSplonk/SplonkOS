#include "lib/pin.h"
#include "lib/proto/uart.h"
#define SPLONKLIB_MILIS_CONST
#include "lib/time.h"
#include <avr/interrupt.h>
#include <util/atomic.h>

volatile milis_t MILIS = 0;
// this is a list of hooks that will be called every tick
static t_ontick_func ONTICK_FUNCS[__T_ONTICK_SLOT_LAST] = {};

milis_t uptime(void) {
	milis_t m;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { m = MILIS; }

	return m;
}

timer_t timer_reset(void) { return (timer_t){.resolve_at = 0}; }

bool timer_wait(timer_t *timer, milis_t delay) {
	if (timer == nullptr)
		return false;
	milis_t current = uptime();
	if (timer->resolve_at == 0)
		timer->resolve_at = current + delay;
	return current >= timer->resolve_at;
}

t_error timer_ontick_register(enum e_timer_ontick_slot slot, t_ontick_func func) {
	if (slot >= __T_ONTICK_SLOT_LAST)
		return ERROR;
	ONTICK_FUNCS[(usize)slot] = func;
	return SUCCESS;
}

#if defined(MCU_atmega32u4) || defined(MCU_atmega328p)

#define TIMER_CLOCK_PRESCALER_VALUE 256UL
#define TIMER_CLOCK_TARGET_HZ		1000UL
#define TIMER_INTERUPT_VECTOR		TIMER0_COMPA_vect
#define TIMER_TOP_MAX				255L

#elif defined(MCU_attiny1614)

#define TIMER_CLOCK_PRESCALER_VALUE 2UL
#define TIMER_CLOCK_TARGET_HZ		1000UL
#define TIMER_INTERUPT_VECTOR		TCB0_INT_vect
#define TIMER_TOP_MAX				65535UL

#else

#error "Unknown MCU for timer purposes"

#endif

#define TIMER_TOP (F_CPU / (TIMER_CLOCK_PRESCALER_VALUE * TIMER_CLOCK_TARGET_HZ) - 1UL)
static_assert(TIMER_TOP < TIMER_TOP_MAX, "TIMER_TOP can't be over TIMER_TOP_MAX");

static void inc_milis(void) { MILIS++; }

void timer_init(void) {
	(void)timer_ontick_register(T_ONTICK_SLOT_MILIS, inc_milis);
#if defined(MCU_atmega32u4) || defined(MCU_atmega328p)

	TCCR0B = 0;
	OCR0A = (u8)TIMER_TOP;
	TCCR0A = _BV(WGM01);
	TIMSK0 = _BV(OCIE0A);
	TCCR0B = _BV(CS02);

#elif defined(MCU_attiny1614)

	TCB0.CTRLA = 0;
	TCB0.CTRLB = TCB_CNTMODE_INT_gc;
	TCB0.CCMP = TIMER_TOP;
	TCB0.INTCTRL = TCB_CAPT_bm;
	TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;

#endif
	sei();
}

ISR(TIMER_INTERUPT_VECTOR) {
	for (usize i = 0; i < tabsize(ONTICK_FUNCS); i++) {
		if (ONTICK_FUNCS[i])
			ONTICK_FUNCS[i]();
	}
#if defined(MCU_attiny1614)
	TCB0.INTFLAGS = TCB_CAPT_bm;
#endif
}

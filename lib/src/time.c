#define SPLONKLIB_MILIS_CONST
#include "lib/time.h"
#include <avr/interrupt.h>
#include <util/atomic.h>

volatile milis_t MILIS = 0;

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

#if defined(MCU_atmega32u4)

#define TIMER_CONTROL_REGA			TCCR0A
#define TIMER_CONTROL_REGB			TCCR0B
#define TIMER_INTERUPT_REG			TIMSK0
#define TIMER_CTC_TOP				OCR0A
#define TIMER_CTC_OUTPUT_NONE		(0)
#define TIMER_CTC_MODE				(_BV(WGM01))
#define TIMER_CLOCK_PRESCALER_VALUE 256UL
#define TIMER_CLOCK_TARGET_HZ		1000UL
#define TIMER_CLOCK_PRESCALER		(_BV(CS02)) // Clk/256
#define TIMER_INTERUPT_BITS			(_BV(OCIE0A))
#define TIMER_INTERUPT_VECTOR		TIMER0_COMPA_vect

#elif defined(MCU_atmega328p)

#define TIMER_CONTROL_REGA			TCCR0A
#define TIMER_CONTROL_REGB			TCCR0B
#define TIMER_INTERUPT_REG			TIMSK0
#define TIMER_CTC_TOP				OCR0A
#define TIMER_CTC_OUTPUT_NONE		(0)
#define TIMER_CTC_MODE				(_BV(WGM01))
#define TIMER_CLOCK_PRESCALER_VALUE 256UL
#define TIMER_CLOCK_TARGET_HZ		1000UL
#define TIMER_CLOCK_PRESCALER		(_BV(CS02)) // Clk/256
#define TIMER_INTERUPT_BITS			(_BV(OCIE0A))
#define TIMER_INTERUPT_VECTOR		TIMER0_COMPA_vect

#else

#error "Unknown MCU for timer purposes"

#endif

#define TIMER_TOP                                                              \
	(F_CPU / (TIMER_CLOCK_PRESCALER_VALUE * TIMER_CLOCK_TARGET_HZ) - 1UL)
static_assert(TIMER_TOP < 255U, "TIMER_TOP can't be over 255");

void timer_init(void) {
	TIMER_CONTROL_REGB = 0;
	TIMER_CTC_TOP = (u8)TIMER_TOP;
	TIMER_CONTROL_REGA = TIMER_CTC_OUTPUT_NONE | TIMER_CTC_MODE;

	TIMER_INTERUPT_REG = TIMER_INTERUPT_BITS;

	// this has to remain at the end. this launches the timer !
	TIMER_CONTROL_REGB = TIMER_CLOCK_PRESCALER;
	sei();
}

ISR(TIMER_INTERUPT_VECTOR) {
	MILIS++;
	// maybe we want to reset if MILIS is near the MAX (aka to not overflow);
}

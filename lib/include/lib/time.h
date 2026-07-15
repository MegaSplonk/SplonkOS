#pragma once

typedef u32 milis_t;
typedef struct {
	milis_t resolve_at;
} timer_t;

#ifndef SPLONKLIB_MILIS_CONST
#define SPLONKLIB_MILIS_CONST const
#endif

extern volatile SPLONKLIB_MILIS_CONST milis_t MILIS;

// TODO: do we want to be able to specify the frequency (in milis) that the
// underlying timer ticks at ?
//
// for example we could say that we have an accuarcy of 2ms
// or even we could try to say an accuarcy of 500us ?
//
// for now, the easiest thing to do is compile time thingy
void timer_init(void);

milis_t uptime(void);
timer_t timer_reset(void);
bool	timer_wait(timer_t *timer, milis_t delay);

typedef void (*t_ontick_func)(void);

enum e_timer_ontick_slot {
	T_ONTICK_SLOT_MILIS,
	T_ONTICK_SLOT_UART,
	T_ONTICK_SLOT_USER1,
	T_ONTICK_SLOT_USER2,
	__T_ONTICK_SLOT_LAST,
};

t_error timer_ontick_register(enum e_timer_ontick_slot slot, t_ontick_func func);

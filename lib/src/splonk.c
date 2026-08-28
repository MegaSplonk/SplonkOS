#include "lib/splonk.h"
#include "lib/proto/i2c.h"

#if !defined(SPLONK) && !defined(SPLONKNOB)
#define SPLONK
#endif

#if defined(SPLONK)
#define SELF_ADDR 0b00011111
#define PEER_ADDR 0b01011111
#elif defined(SPLONKNOB)
#define SELF_ADDR 0b01011111
#define PEER_ADDR 0b00011111
#endif

I2C_SLOT(gcall1, gcall, gcall);

#ifdef SPLONK

I2C_SLOT(splonk1, splonk, splonk);
I2C_SLOT(knob1, splonknob, dummy);

void splonk_init() {
	i2c_register_peer(nullptr, &gcall1, 0);
	i2c_register_peer(nullptr, &splonk1, SELF_ADDR);
	i2c_register_peer(nullptr, &knob1, PEER_ADDR);
	i2c_set_addr(SELF_ADDR, 1);
}

#elifdef SPLONKNOB

I2C_SLOT(knob1, dummy, splonknob);

void splonk_init() {
	i2c_register_peer(nullptr, &gcall1, 0);
	i2c_register_peer(nullptr, &knob1, SELF_ADDR);
	i2c_set_addr(SELF_ADDR, 1);
}

#endif

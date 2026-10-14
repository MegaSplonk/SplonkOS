#include "lib/ec11.h"
#include "lib/pinout.h"
#include "lib/proto/i2c.h"
#include "lib/proto/uart.h"
#include "lib/splonk.h"
#include "lib/time.h"

#include <avr/io.h>
#include <util/atomic.h>
#include <util/delay.h>

#define SELF_ADDR 0b01011111

I2C_SLOT(gcall1, gcall, gcall);

i2c_peer *self;

I2C_SLOT(knob1, dummy, splonknob);

void splonk_loop() {
	timer_init();
	uart_init();
	debug_print("%s\n", "I'm splonknob");
	i2c_register_peer(nullptr, &gcall1, (ipi){});
	i2c_register_peer(&self, &knob1, (ipi){SELF_ADDR, IPD_NONE, IPT_SPLONKNOB});
	i2c_set_addr(SELF_ADDR, 1);
	i2c_init();
	i2c_listen();
	debug_print("SELF %d type%d", self->info.addr, self->info.type);

	timer_t		t = timer_reset();
	splonknob	sk = {};
	timer_t		tt = timer_reset();
	timer_t		ttt = timer_reset();
	timer_t		tttt = timer_reset();
	struct ec11 ec_l, ec_r;
	ec11_init(&ec_l, (pin)L_A, (pin)L_B, (pin)L_BTN);
	ec11_init(&ec_r, (pin)R_A, (pin)R_B, (pin)R_BTN);
	splonknob last = {};
	// add second ec11 when wired in
	while (true) {
		switch (ec11_poll(&ec_l, &sk.butn_l)) {
		case (ROT_A):
			sk.knob_l += 1;
			break;
		case (ROT_B):
			sk.knob_l -= 1;
			break;
		case (ROT_NONE):
			break;
		}
		switch (ec11_poll(&ec_r, &sk.butn_r)) {
		case (ROT_A):
			sk.knob_r += 1;
			break;
		case (ROT_B):
			sk.knob_r -= 1;
			break;
		case (ROT_NONE):
			break;
		}
		if (timer_wait(&t, 5)) {
			t = timer_reset();
			if (grb_avail(&self->io->output) >= sizeof(sk)) {
				i2c_send(self, (void *)&sk, sizeof(sk));
				sk = (splonknob){.butn_l = sk.butn_l, .butn_r = sk.butn_r};
			}
		}
		if (timer_wait(&tt, 5)) {
			tt = timer_reset();
			bool same = true;
			for (usize i = 0; i < sizeof(sk); i++)
				if (((u8 *)&sk)[i] != ((u8 *)&last)[i])
					same = false;
			if (!same) {
				for (usize i = 0; i < sizeof(sk); i++)
					uart_send_u8_hex(((u8 *)&sk)[i]);
				uart_sendstring("\r\n");
				last = sk;
			}
		}
		if (timer_wait(&ttt, 60000)) { // for debugging purposes
			soft_reset();
		}
		if (timer_wait(&tttt, 2000)) { // for debugging purposes
      tttt = timer_reset();
			i2c_debug();
		}
	}
}

int main(void) {
	splonk_loop();
	return 0;
}

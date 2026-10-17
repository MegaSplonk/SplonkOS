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
	timer_t		i2c_poll_timer = timer_reset();
	struct ec11 ec_l, ec_r;
	ec11_init(&ec_l, (pin)L_A, (pin)L_B, (pin)L_BTN);
	ec11_init(&ec_r, (pin)R_A, (pin)R_B, (pin)R_BTN);

	i8		  knob_l = 0, knob_r = 0;
	bool	  btn_l = false, btn_r = false;
	splonknob last = {};
	// add second ec11 when wired in
	while (true) {
		switch (ec11_poll(&ec_l, &btn_l)) {
		case (ROT_A):
			if (knob_l < 120)
				knob_l++;
			break;
		case (ROT_B):
			if (knob_l > -120)
				knob_l--;
			if (knob_l < -120)
				knob_l = -120;
			break;
		case (ROT_NONE):
			break;
		}
		switch (ec11_poll(&ec_r, &btn_r)) {
		case (ROT_A):
			if (knob_r < 120)
				knob_r++;
			break;
		case (ROT_B):
			if (knob_r > -120)
				knob_r--;
			if (knob_r < -120)
				knob_r = -120;
			break;
		case (ROT_NONE):
			break;
		}
		sk.l1 = knob_l > 0;
		sk.r1 = knob_l < 0;
		sk.b1 = btn_l;
		sk.l2 = knob_r > 0;
		sk.r2 = knob_r < 0;
		sk.b2 = btn_r;

		if (timer_wait(&t, 5)) {
			t = timer_reset();
			if (i2c_send(self, (void *)&sk, sizeof(sk)) == SUCCESS) {
				// One bit carries one step. Keep remaining motion (and retain all
				// motion on backpressure) instead of collapsing an entire burst.
				if (knob_l > 0)
					knob_l--;
				else if (knob_l < 0)
					knob_l++;
				if (knob_r > 0)
					knob_r--;
				else if (knob_r < 0)
					knob_r++;
			}
		}
		if (timer_wait(&tt, 5)) {
			tt = timer_reset();
			if (sk.raw ^ last.raw) {
				for (usize i = 0; i < sizeof(sk); i++)
					uart_send_u8_hex(((u8 *)&sk)[i]);
				uart_sendstring("\r\n");
				last = sk;
			}
		}
		if (timer_wait(&i2c_poll_timer, 1)) {
			i2c_poll_timer = timer_reset();
			i2c_poll();
		}
		// if (timer_wait(&tttt, 2000)) { // for debugging purposes
		//     tttt = timer_reset();
		// 	i2c_debug();
		// }
	}
}

int main(void) {
	splonk_loop();
	return 0;
}

#include "lib/splonk.h"
#include "lib/ec11.h"
#include "lib/pinout.h"
#include "lib/proto/i2c.h"
#include "lib/proto/uart.h"
#include "lib/time.h"

#include <avr/io.h>
#include <util/atomic.h>
#include <util/delay.h>

#if !defined(SPLONK) && !defined(SPLONKNOB)
#define SPLONK
#endif

#if defined(SPLONK)
#define SELF_ADDR 0b00011111
#elif defined(SPLONKNOB)
#define SELF_ADDR 0b01011111
#endif

I2C_SLOT(gcall1, gcall, gcall);

#ifdef SPLONK

i2c_peer *self;
i2c_peer *otherhalf;
i2c_peer *peer1;

I2C_SLOT(splonk1, splonk, splonk);
typedef splonk splonk_;
I2C_SLOT(splonk2, splonk_, splonk_);
I2C_SLOT(knob1, splonknob, dummy);

static void knob1_input_to_uart() { // can be used for debugging for now i guess
	if (!peer1 || !peer1->io)
		return;

	usize	  read_bytes = 0;
	splonknob data;
	static splonknob last;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		if (grb_count(&peer1->io->input) >= sizeof(data)) {
			grb_read(&peer1->io->input, (void *)&data, sizeof(data), &read_bytes);
			bool same = true;
			for (usize i = 0; i < sizeof(data); i++)
				if (((u8 *)&data)[i] != ((u8 *)&last)[i])
					same = false;
			if (!same) {
				if (data.knob_l) {
					uart_sendstring("LK");
					i16 knob_l = data.knob_l;
					uart_send_i16(knob_l);
				}
				if (data.butn_l) {
					uart_sendstring("LB");
					uart_send_u8(data.butn_l);
				}
				if (data.knob_r) {
					uart_sendstring("RK");
					i16 knob_r = data.knob_r;
					uart_send_i16(knob_r);
				}
				if (data.butn_r) {
					uart_sendstring("RB");
					uart_send_u8(data.butn_r);
				}
				uart_sendstring("\r\n");
				for (usize i = 0; i < sizeof(data); i++)
					uart_send_u8_hex(((u8 *)&data)[i]);
				uart_sendstring("\r\n");
				last = data;
			}
		}
	}
}

void splonk_loop() {
	timer_init();
	uart_init();
	debug_print("%s\n", "I'm alive");
	i2c_register_peer(nullptr, &gcall1, (ipi){});
	i2c_register_peer(&self, &splonk1, (ipi){SELF_ADDR, IPD_ALL, IPT_SPLONK});
  i2c_register_slot(&splonk2, IPT_SPLONK);
  i2c_register_slot(&knob1, IPT_SPLONKNOB); // add more slots as needed
	i2c_init();
	i2c_set_addr(SELF_ADDR, 1);
	i2c_listen();
	timer_ontick_register(T_ONTICK_SLOT_I2C_PEER, knob1_input_to_uart);
	// timer_ontick_register(T_ONTICK_SLOT_I2C_SELF, self_input_to_uart);
	debug_print("SELF %d, PEER %d", self->info.addr, peer1->info.addr);
	// debug_print("I'm alive %p, %d, %p", (void *)peer1, peer1->addr, (void *)peer1->io);
	timer_t	  t = timer_reset();
	// timer_t	  tt = timer_reset();
	timer_t	  ttt = timer_reset();
	splonknob sk = {};
	// splonk sp = {};

	while (true) {
		if (timer_wait(&t, 10)) {
			t = timer_reset();
      i2c_get_peer(&peer1, (ipi){0,0,IPT_SPLONKNOB});
			i2c_query(peer1, NULL, sizeof(sk));
			//    splonk self_data = {.keys = {}, .led_mode = 0, .peers = {}};
			//    for (usize i = 0; i < tabsize(self_data.peers); i++)
			//      self_data.peers[i] = self->downstream[i] ? self->downstream[i]->info : (ipi){};
			//    i2c_send(self, (u8 *)&self_data, sizeof(splonk));
			// if (otherhalf && i2c_query(otherhalf, (u8 *)&sp, sizeof(sp)) == SUCCESS) {
			//   for (usize i = 0; i < tabsize(self_data.peers); i++) {
			//        i2c_peer *downstream = NULL;
			//        i2c_register(&downstream, sp.peers[i]);
			//        otherhalf->downstream[i] = downstream;
			//      }
			//    }
		}
		// if (timer_wait(&tt, 5000)) {
		// 	tt = timer_reset();
		// 	i2c_gcall(&(gcall){.use_addr = {.instruction = GCI_USE_ADDR, PEER_ADDR + 1}});
		// }
		if (timer_wait(&ttt, 30000)) {
			ttt = timer_reset();
			i2c_gcall(&(gcall){.reset = {.instruction = GCI_RESET, 0}});
			debug_print("RESET PEERS");
		}
	}
}

#elifdef SPLONKNOB

i2c_peer *self;

#if defined(MCU_atmega328p) // for testing purposes

#define R_BTN D2
#define R_A	  D3
#define R_B	  D4

#elif defined(MCU_attiny1614)

#define R_BTN P13
#define R_A	  P12
#define R_B	  P11

#endif

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
	struct ec11 ec;
	ec11_init(&ec, (pin)R_A, (pin)R_B, (pin)R_BTN);
	splonknob last = {};
	// add second ec11 when wired in
	while (true) {
		switch (ec11_poll(&ec, &sk.butn_l)) {
		case (ROT_A):
			sk.knob_l += 1;
			break;
		case (ROT_B):
			sk.knob_l -= 1;
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
	}
}

#elifdef SPLONKO
#elifdef SPLONKREEN
#elifdef SPLONKPAD
#elifdef SPLONKBAR

#else

void splonk_loop() {
	uart_init();

	while (true) {
		uart_sendstring("dumbass\r\n");
	}
}

#endif

#include "lib/grb.h"
#include "lib/proto/i2c.h"
#include "lib/proto/uart.h"
#include "lib/splonk.h"
#include "lib/pinout.h"
#include "lib/ec11.h"
#include "lib/time.h"
#include <avr/io.h>
#include <util/atomic.h>
#include <util/delay.h>

i2c_peer *self;
i2c_peer *peer1;

#ifdef SPLONK



#elifdef SPLONKNOB

#if defined(MCU_atmega328p) // for testing purposes

#define R_BTN D2
#define R_A	  D3
#define R_B	  D4

#elif defined(MCU_attiny1614)

#define R_BTN P13
#define R_A	  P12
#define R_B	  P11

#endif


#endif

static void peer_input_to_uart(i2c_peer *peer, const char *identifier) {
	if (!peer || !peer->io)
		return;

	usize	  read_bytes = 0;
	// splonknob data;
	u8 data[4];
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    if (grb_count(&peer->io->input) >= sizeof(data)) {
      uart_send_u16(grb_count(&peer->io->input));
      grb_read(&peer->io->input, (void *)&data, sizeof(data), &read_bytes);
      if (read_bytes == sizeof(data)) {
        // if (data.knob_l) {
        //   uart_sendstring("LK");
        //   i16 knob_l = data.knob_l;
        //   uart_send_i16(knob_l);
        // }
        // if (data.butn_l) {
        //   uart_sendstring("LB");
        //   uart_send_u8(data.butn_l);
        // } // FIXME broken
        // if (data.knob_l) {
        //   uart_sendstring("RK");
        //   i16 knob_r = data.knob_r;
        //   uart_send_i16(knob_r);
        // }
        // if (data.butn_r) {
        //   uart_sendstring("RB");
        //   uart_send_u8(data.butn_r);
        // } // FIXME broken
        // uart_sendstring("\r\n");
        for (usize i=0; i < sizeof(data); i++)
          // uart_send_u8_hex(((u8*)&data)[i]);
          uart_send_u8_hex(data[i]);
        uart_sendstring("\r\n");
      }
    }
  }
}

static void peer1_input_to_uart(void) { peer_input_to_uart(peer1, "Queried from peer1: "); }

static void self_input_to_uart(void) { peer_input_to_uart(self, "Recv'd: "); }

int main(void) {
	timer_init();
	uart_init();
	debug_print("%s\n", "I'm alive");
	i2c_init();
	splonk_init();
	i2c_listen();
	i2c_register_peer(&self, nullptr, I2C_SELF_IDX);
	i2c_register_peer(&peer1, nullptr, I2C_SELF_IDX + 1);
	timer_ontick_register(T_ONTICK_SLOT_I2C_PEER, peer1_input_to_uart);
	timer_ontick_register(T_ONTICK_SLOT_I2C_SELF, self_input_to_uart);
	debug_print("SELF %d, PEER %d", self->addr, peer1->addr);
	// debug_print("I'm alive %p, %d, %p", (void *)peer1, peer1->addr, (void *)peer1->io);
	timer_t t = timer_reset();
  splonknob sk = {};

#ifdef SPLONK
  while (true) {
		if (timer_wait(&t, 10)) {
			t = timer_reset();
      i2c_query(peer1, NULL, sizeof(sk));
		}
	}

#elifdef SPLONKNOB

	timer_t tt = timer_reset();
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
      usize w;
      if (grb_avail(&self->io->output) >= sizeof(sk)) {
        // i2c_send(self, (void*)&sk, sizeof(sk));
        i2c_send(self, "abcdefghijk", sizeof(sk));
        sk = (splonknob){ .butn_l = sk.butn_l, .butn_r = sk.butn_r };
      }
		}
		if (timer_wait(&tt, 5)) {
			tt = timer_reset();
      bool same = true;
      for (usize i=0; i < sizeof(sk); i++)
        if (((u8*)&sk)[i] != ((u8*)&last)[i])
          same = false;
      if (!same) {
        for (usize i=0; i < sizeof(sk); i++)
          uart_send_u8_hex(((u8*)&sk)[i]);
        uart_sendstring("\r\n");
        last = sk;
      }
    }
	}
#endif


	return 0;
}


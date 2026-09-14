#include "lib/grb.h"
#include "lib/proto/i2c.h"
#include "lib/proto/uart.h"
#include "lib/splonk.h"
#include "lib/pinout.h"
#include "lib/ec11.h"
#include "lib/time.h"
#include <avr/io.h>
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
	splonknob data;
	if (grb_count(&peer->io->input) >= sizeof(data)) {
		grb_read(&peer->io->input, (void *)&data, sizeof(data), &read_bytes);
		if (read_bytes == sizeof(data)) {
			uart_sendstring(identifier);
      uart_sendstring("LK");
      uart_send_i16(data.knob_l);
      uart_sendstring("LB");
      uart_send_u8(data.butn_l);
      uart_sendstring("RK");
      uart_send_i16(data.knob_r);
      uart_sendstring("RB");
      uart_send_u8(data.butn_r);
			uart_sendstring("\r\n");
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
		if (timer_wait(&t, 10000)) {
			t = timer_reset();
      i2c_query(peer1, (u8 *)&sk, sizeof(sk));
      sk = (splonknob){};
		}
	}

#elifdef SPLONKNOB

  struct ec11 ec;
	ec11_init(&ec, (pin)R_A, (pin)R_B, (pin)R_BTN);
  // add second ec11 when wired in
  while (true) {
    switch (ec11_poll(&ec, (bool*)&sk.butn_l)) {
		case (ROT_A):
      sk.knob_l += 1;
      break;
    case (ROT_B):
      sk.knob_l -= 1;
      break;
    case (ROT_NONE):
      break;
    }
		if (timer_wait(&t, 2)) {
			t = timer_reset();
      usize w;
      grb_write(&self->io->output, (void*)&sk, sizeof(sk), &w);
      sk = (splonknob){};
		}
	}
#endif


	return 0;
}


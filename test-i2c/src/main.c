#include "lib/grb.h"
#include "lib/proto/i2c.h"
#include "lib/proto/uart.h"
#include "lib/time.h"
#include <avr/io.h>
#include <util/delay.h>

#ifndef TEST1
#define TEST0
#endif

#if defined(TEST0)
#define SELF_ADDR 0b00011111
#define PEER_ADDR 0b10011111
#define MSG		  "ping"
#define DELAY 2000
#elif defined(TEST1)
#define SELF_ADDR 0b10011111
#define PEER_ADDR 0b00011111
#define MSG		  "pong"
#define DELAY 5000
#endif

GRBIO_INIT(peer1io, _peer1i, _peer1o, 64, 64);

i2c_peer *self;
i2c_peer *peer1;

static void peer_input_to_uart(i2c_peer *peer, const char *identifier) {
	if (!peer || !peer->io)
		return;

	u8 data[65];
  usize read_bytes = 0;
  if (grb_count(&peer->io->input)) {
    grb_read(&peer->io->input, data, 64, &read_bytes);
    data[read_bytes] = 0;
    if (read_bytes) {
      uart_send_u8((u8)read_bytes);
      // uart_send_u32((u32)grb_count(&peer->io->input));
      uart_sendstring(identifier);
      uart_sendstring((const char *)data);
      uart_sendstring("\r\n");
    }
  }
}

static void peer1_input_to_uart(void) {
  peer_input_to_uart(peer1, "Queried from peer1: ");
}

static void self_input_to_uart(void) {
  peer_input_to_uart(self, "Recv'd: ");
}

int main(void) {
	timer_init();
	uart_init();
	debug_print("%s\n", "I'm alive");
	i2c_init();
	i2c_set_addr(SELF_ADDR, 1);
	i2c_listen();
	i2c_register_peer(&peer1, &peer1io, PEER_ADDR);
	i2c_register_peer(&self, nullptr, 0);
	timer_ontick_register(T_ONTICK_SLOT_I2C_PEER, peer1_input_to_uart);
	timer_ontick_register(T_ONTICK_SLOT_I2C_SELF, self_input_to_uart);
	debug_print("I'm alive %p, %d, %p", (void *)peer1, peer1->addr, (void *)peer1->io);
	timer_t t = timer_reset();
	// timer_t tp = timer_reset();


	while (true) {
    // if (timer_wait(&tp, 1)) {
    //   tp = timer_reset();
      // peer1_input_to_uart();
    // uart_poll();
    // i2c_poll();
    // }
		if (timer_wait(&t, 4000 + DELAY)) {
      t = timer_reset();
			char *yeah = "Hello!" MSG;
			i2c_send(peer1, (u8 *)yeah, 10);
	    // debug_print("I'm alive %p, %d, %p", (void *)peer1, peer1->addr, (void *)peer1->io);
		}
	}

	return 0;
}


// void test1() {
// 	static milis_t ms = 0;
// 	if (++ms >= 3000) {
// 		ms = 0;
// 		debug_print("THREE SECONDS");
// 	}
// }
//
// int main() {
//   timer_init();
// 	uart_init();
// 	debug_print("I'm alive %p, %d, %p", (void *)peer1, peer1->addr, (void *)peer1->io);
// 	timer_ontick_register(T_ONTICK_SLOT_I2C_PEER, test1);
// 	timer_t t = timer_reset();
// 	timer_t tp = timer_reset();
//
// 	while (true) {
//     if (timer_wait(&tp, 1)) {
//       tp = timer_reset();
//       // peer1_input_to_uart();
//     uart_poll();
//     // i2c_poll();
//     }
// 		if (timer_wait(&t, 5000)) {
//       t = timer_reset();
// 	    debug_print("FIVE SECONDS");
// 		}
// 	}
//
// 	return 0;
// }


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
#define MSG		  "zero"
#elif defined(TEST1)
#define SELF_ADDR 0b10011111
#define PEER_ADDR 0b00011111
#define MSG		  " one"
#endif

GRBIO_INIT(peer1io, _peer1i, _peer1o, 64, 64);

i2c_peer *peer1;

static void peer1_input_to_uart(void) {
	if (!peer1 || !peer1->io)
		return;

	u8 data[17];
	for (; grb_count(&peer1->io->input);) {
		usize read_bytes = 0;
		grb_read(&peer1->io->input, data, 16, &read_bytes);
		data[read_bytes] = 0;
		uart_sendstring((const char *)data);
	}
}

int main(void) {
	timer_init();
	uart_init();
	debug_print("%s\n", "I'm alive");
	i2c_init();
	timer_ontick_register(T_ONTICK_SLOT_I2C_PEER, peer1_input_to_uart);
	i2c_set_addr(SELF_ADDR, 1);
	i2c_listen();
	i2c_register_peer(&peer1, &peer1io, PEER_ADDR);
	timer_t t = timer_reset();
	// timer_t tp = timer_reset();

	while (true) {
    // if (timer_wait(&tp, 1)) {
    //   peer1_input_to_uart();
    // uart_poll();
    // i2c_poll();
    // }
		if (timer_wait(&t, 5000)) {
      t = timer_reset();
			debug_print("I'm alive %p, %d, %p", (void *)peer1, peer1->addr, (void *)peer1->io);
			char *yeah = "Hello!" MSG;
			i2c_send_async(peer1, (u8 *)yeah, 10);
      // i2c_send(peer1->addr, (u8 *)yeah, 10);
		}
	}

	return 0;
}

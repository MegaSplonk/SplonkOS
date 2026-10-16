#include "lib/ec11.h"
#include "lib/key_matrix.h"
#include "lib/pinout.h"
#include "lib/proto/i2c.h"
#include "lib/proto/uart.h"
#include "lib/rgb_matrix.h"
#include "lib/splonk.h"
#include "lib/time.h"

#include <avr/io.h>
#include <util/atomic.h>
#include <util/delay.h>

#define SELF_ADDR 0b01100000

I2C_SLOT(gcall1, gcall, gcall);

i2c_peer *self;

I2C_SLOT(splonko1, rgb_control, splonko);

static void update_rgb_matrix() {
	if (!self || !self->io)
		return;

	usize		read_bytes = 0;
	rgb_control data;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		if (grb_count(&self->io->input) < sizeof(data))
			return;
		grb_read(&self->io->input, (void *)&data, sizeof(data), &read_bytes);
	}
	rm_set_mode(data.led_mode);
}

void splonk_loop() {
	timer_init();
	uart_init();
	debug_print("%s\n", "I'm splonko");
	i2c_register_peer(nullptr, &gcall1, (ipi){});
	i2c_register_peer(&self, &splonko1, (ipi){SELF_ADDR, IPD_NONE, IPT_SPLONKO});
	i2c_set_addr(SELF_ADDR, 1);
	i2c_init();
	i2c_listen();
	debug_print("SELF %d type%d", self->info.addr, self->info.type);
	rm_init();
	km_init();
	timer_t led_updt_timer = timer_reset();
	timer_t i2c_send_timer = timer_reset();
	timer_t reset_timer = timer_reset();
	timer_t i2c_poll_timer = timer_reset();
	splonko so = {};
	splonko last = {};
	rm_set_keys(so.keys);

	while (true) {
		km_scan(so.keys, sizeof(so.keys));

		if (timer_wait(&led_updt_timer, 5)) {
			led_updt_timer = timer_reset();
			update_rgb_matrix();
			rm_update();
		}
		if (timer_wait(&i2c_send_timer, 5)) {
			i2c_send_timer = timer_reset();
			if (so.keys[0] ^ last.keys[0]) {
				for (usize i = 0; i < sizeof(so); i++)
					uart_send_u8_hex(((u8 *)&so)[i]);
				uart_sendstring("\r\n");
				i2c_send(self, (void *)&so, sizeof(so));
				last = so;
			}
		}
		if (timer_wait(&reset_timer, 60000)) {
			soft_reset();
		}
		if (timer_wait(&i2c_poll_timer, 1)) {
			i2c_poll_timer = timer_reset();
			i2c_poll();
		}
	}
}

int main(void) {
	splonk_loop();
	return 0;
}

#include "lib/key_matrix.h"
#include "lib/proto/i2c.h"
#include "lib/proto/uart.h"
#include "lib/rgb_matrix.h"
#include "lib/splonk.h"
#include "lib/time.h"
#include <util/atomic.h>

#define SELF_ADDR 0x60

I2C_SLOT(gcall1, gcall, gcall);
I2C_SLOT(splonkus1, rgb_control, splonkus);

int main(void) {
	timer_init();
	uart_init();
	i2c_peer *self;
	i2c_register_peer(nullptr, &gcall1, (ipi){});
	i2c_register_peer(&self, &splonkus1, (ipi){SELF_ADDR, IPD_NONE, IPT_SPLONKUS});
	i2c_init();
	i2c_set_addr(SELF_ADDR, true);
	i2c_listen();

	splonkus snapshot = {};
	km_init();
	rm_set_keys(snapshot.keys);
	rm_init();
	timer_t publish_timer = timer_reset();
	timer_t poll_timer = timer_reset();
	while (true) {
		if (km_scan(snapshot.keys, sizeof(snapshot.keys)) == SUCCESS ||
			timer_wait(&publish_timer, 10)) {
			// Publish complete levels, including releases, even without changes.
			// The I2C slave latches a complete snapshot at each addressed read.
			if (i2c_send(self, (u8 *)&snapshot, sizeof(snapshot)) == SUCCESS)
				publish_timer = timer_reset();
		}
		if (timer_wait(&poll_timer, 1)) {
			poll_timer = timer_reset();
			i2c_poll(); // Process host general calls, including address assignment.
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
				while (grb_count(&self->io->input) >= sizeof(rgb_control)) {
					rgb_control command;
					grb_read(&self->io->input, &command, sizeof(command), nullptr);
					if (command.led_mode < RM_MODE_COUNT)
						rm_set_mode((led_mode)command.led_mode);
				}
			}
			rm_update();
		}
	}
}

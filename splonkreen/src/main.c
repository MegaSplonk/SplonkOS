#include "lib/pin.h"
#include "lib/pinout.h"
#include "lib/proto/i2c.h"
#include "lib/proto/uart.h"
#include "lib/splonk.h"
#include "lib/splonkreen.h"
#include "lib/time.h"

#include <avr/io.h>
#include <util/atomic.h>
#include <util/delay.h>

#define SELF_ADDR 0b01011111

I2C_SLOT(gcall1, gcall, gcall);

i2c_peer *self;

I2C_SLOT(splonkreen_, splonkreen, dummy);

void splonk_loop() {

	timer_t	   t = timer_reset();
	splonkreen sk = {};
	usize	   r;
	send_sentence("chibrolux ", 4, 0);
	send_sentence("chibrolax", 5, 0);
	send_sentence("chibrolynx", 6, 0);


	draw(0, 0, maieul);

	display_screen();
	// add second ec11 when wired in
	while (true) {
		if (timer_wait(&t, 5)) {
			t = timer_reset();
			if (grb_avail(&self->io->input) >= sizeof(sk)) {
				// i2c_(self, (void *)&sk, sizeof(sk));
				r = 0;
				if (!grb_read(&self->io->input, &sk, sizeof(sk), &r) && r == sizeof(sk)) {
					// here sk is fully valid
				}
			}
		}
	}
}

int main(void) {
	timer_init();
	uart_init();
	debug_print("%s\n", "I'm splonkreen_");
	i2c_register_peer(nullptr, &gcall1, (ipi){});
	i2c_register_peer(&self, &splonkreen_, (ipi){SELF_ADDR, IPD_NONE, IPT_SPLONKREEN});
	i2c_set_addr(SELF_ADDR, 1);
	i2c_init();
	i2c_listen();
	spi_init();
	screen_init();
	debug_print("SELF %d type%d", self->info.addr, self->info.type);
	splonk_loop();
	return 0;
}

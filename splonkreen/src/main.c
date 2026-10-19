#include "lib/lib.h"
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

I2C_SLOT(screen, splonkreen, dummy);

void splonk_loop() {

	timer_t t = timer_reset();
	timer_t i2c_poll_timer = timer_reset();
	// timer_t	   test = timer_reset();
	splonkreen sk = {};
	bool	   mod_layout_horizontal = false;
	usize	   r;

	clear_screen();

	draw(0, 0, SPLONKREEN_DEFAULT_IMAGE);

	const u8 image_width = SPLONKREEN_DEFAULT_IMAGE.width;
	const u8 remaining_width = (128 - image_width);
	const u8 mod_offset =
		image_width + (u8)((remaining_width / 2) - (8 * ((__IMG_MOD_SIZE - 1) / 2)));
	const u8 mod_page_l = 0;
	const u8 mod_page_r = 6;
	const u8 lay_offset =
		image_width + (u8)((remaining_width / 2) - (image_layers[IMG_LAY_DEF]->width / 2));
	const u8 lay_page = 2;

	draw(lay_page, lay_offset, *image_layers[IMG_LAY_DEF]);
	display_screen();
	while (true) {
		if (timer_wait(&t, 5)) {
			t = timer_reset();
			if (grb_avail(&self->io->input) < sizeof(sk))
				continue;
			r = 0;
			if (!grb_read(&self->io->input, &sk, sizeof(sk), &r) && r == sizeof(sk)) {
				for (u8 idx = IMG_MOD_ERASE + 1; idx < __IMG_MOD_SIZE; idx++) {
					u8 page, offset;
					if (mod_layout_horizontal) {
						page = (idx - 1) < (__IMG_MOD_SIZE - 1) / 2 ? mod_page_l : mod_page_r;
						offset = mod_offset + (u8)(((idx - 1) % ((__IMG_MOD_SIZE - 1) / 2)) * 16);
					} else {
						page = mod_page_l + (u8)((idx - 1) % ((__IMG_MOD_SIZE - 1) / 2)) * 2;
						offset = (u8)(mod_offset + (((idx - 1) < (__IMG_MOD_SIZE - 1) / 2)
														? 0
														: image_modifiers[IMG_MOD_ERASE]->width +
															  image_layers[IMG_LAY_DEF]->width));
					}
					if (!(sk.mods & _BV(idx - 1))) {
						draw(page, offset, *image_modifiers[IMG_MOD_ERASE]);
						continue;
					}
					draw(page, offset, *image_modifiers[idx]);
				}
				draw(lay_page, lay_offset, *image_layers[sk.layers]);
				display_screen();
			}
		}
		if (timer_wait(&i2c_poll_timer, 1)) {
			i2c_poll_timer = timer_reset();
			i2c_poll();
		}
		// if (timer_wait(&test, 500)) {
		// 	test = timer_reset();
		// 	usize			  w;
		// 	static splonkreen testkreen;
		// 	grb_write(&self->io->input, &testkreen, sizeof(testkreen), &w);
		// 	testkreen.mods = (u8)(testkreen.mods + 1) % _BV(__IMG_MOD_SIZE - 1);
		// 	testkreen.layers = (u8)(testkreen.layers + 1) % __IMG_LAY_SIZE;
		//     mod_layout_horizontal = !mod_layout_horizontal;
		// }
	}
}

int main(void) {
	timer_init();
	uart_init();
	debug_print("%s\n", "I'm splonkreen");
	i2c_register_peer(nullptr, &gcall1, (ipi){});
	i2c_register_peer(&self, &screen, (ipi){SELF_ADDR, IPD_NONE, IPT_SPLONKREEN});
	i2c_set_addr(SELF_ADDR, 1);
	i2c_init();
	i2c_listen();
	spi_init();
	screen_init();
	debug_print("SELF %d type%d", self->info.addr, self->info.type);
	splonk_loop();
	return 0;
}

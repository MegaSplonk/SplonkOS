
#include "lib/font_8x6.h"
#include "lib/splonkreen.h"

int main(void) {

	screen_init();

	send_sentence("chibrolux ", 4, 0);
	send_sentence("chibrolax", 5, 0);
	send_sentence("chibrolynx", 6, 0);

	// draw(0, 0, maieul);

	display_screen();
	_delay_ms(5000);
	send_command(0xAE);
	while (1) {
	}
}

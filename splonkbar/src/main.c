#include "lib/pin.h"
#include "lib/pinout.h"
#include "lib/proto/adc.h"
#include "lib/proto/i2c.h"
#include "lib/proto/uart.h"
#include "lib/splonk.h"
#include "lib/time.h"

#include <avr/io.h>
#include <util/atomic.h>
#include <util/delay.h>

#define SELF_ADDR 0b01011111

I2C_SLOT(gcall1, gcall, gcall);

i2c_peer *self;

I2C_SLOT(bar1, dummy, splonkbar);

void splonk_loop() {
	timer_init();
	uart_init();
	adc_init(ADC_AVCC, ADC_PRESCALER_16);
	debug_print("%s\n", "I'm splonkbar");
	i2c_register_peer(nullptr, &gcall1, (ipi){});
	i2c_register_peer(&self, &bar1, (ipi){SELF_ADDR, IPD_NONE, IPT_SPLONKBAR});
	i2c_set_addr(SELF_ADDR, 1);
	i2c_init();
	i2c_listen();
	debug_print("SELF %d type%d", self->info.addr, self->info.type);

	timer_t	  t = timer_reset();
	splonkbar sb = {};
	timer_t	  tt = timer_reset();
	timer_t	  ttt = timer_reset();

	splonkbar last = {};
	while (true) {
		sb.value = adc_read_pin_10bit(ADC_ADC4);
		if (timer_wait(&t, 5)) {
			t = timer_reset();
			// TODO - use ISR instead of timers and only fill buffer when result is available
			// and only request measure when buffer is emptied/not full
			if (grb_avail(&self->io->output) >= sizeof(sb)) {
				i2c_send(self, (void *)&sb, sizeof(sb));
				// sb = (splonkbar){};
			}
		}
		if (timer_wait(&tt, 5)) {
			tt = timer_reset();
			bool same = true;
			for (usize i = 0; i < sizeof(sb); i++)
				if (((u8 *)&sb)[i] != ((u8 *)&last)[i])
					same = false;
			if (!same) {
				for (usize i = 0; i < sizeof(sb); i++)
					uart_send_u8_hex(((u8 *)&sb)[i]);
				uart_sendstring("\t");
				uart_send_u16(sb.value);
				uart_sendstring("\r\n");
				last = sb;
			}
		}
		if (timer_wait(&ttt, 30000)) { // for debugging purposes
			soft_reset();
		}
	}
}

int main(void) {
	splonk_loop();
	return 0;
}

#include "lib/pin.h"
#include "lib/pinout.h"
#include "lib/proto/adc.h"
#include "lib/proto/i2c.h"
#include "lib/proto/uart.h"
#include "lib/splonk.h"
#include "lib/time.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/delay.h>

#define SELF_ADDR 0b01011111

I2C_SLOT(gcall1, gcall, gcall);

i2c_peer *self;

static volatile u16  adc_sample;
static volatile bool adc_sample_ready;

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

	ADC0.INTCTRL = ADC_RESRDY_bm;
	timer_t sample_timer = timer_reset();
	timer_t debug_timer = timer_reset();

	while (true) {
		bool sample_ready = false;
		u16 sample = 0;
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			if (adc_sample_ready) {
				sample = adc_sample;
				adc_sample_ready = false;
				sample_ready = true;
			}
		}

		if (sample_ready) {
			splonkbar packet = {.value = sample};
			(void)i2c_send(self, (u8 *)&packet, sizeof(packet));
		}

		if (timer_wait(&sample_timer, 5)) {
			sample_timer = timer_reset();
			ADC0.MUXPOS = ADC_ADC4;
			ADC0.COMMAND = ADC_STCONV_bm;
		}

		if (sample_ready && timer_wait(&debug_timer, 100)) {
			debug_timer = timer_reset();
			debug_print("ADC %u\n", sample);
		}
	}
}

ISR(ADC0_RESRDY_vect) {
	adc_sample = ADC0.RES;
	adc_sample_ready = true;
	ADC0.INTFLAGS = ADC_RESRDY_bm;
}

int main(void) {
	splonk_loop();
	return 0;
}

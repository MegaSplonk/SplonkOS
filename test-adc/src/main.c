#include "lib/proto/adc.h"
#include "lib/proto/uart.h"
#include <avr/io.h>
#include <util/delay.h>

static uint8_t read_pot(uint8_t start, uint8_t end) {
	uint16_t d_pot = adc_read_pin_10bit(ADC_ADC0);
	// uart_send_u16(d_pot), uart_sendstring("         \n"), uart_tx('\r');
	return (uint8_t)(((uint32_t)d_pot * ((uint16_t)end - (uint16_t)start + 1u)) / 1024u);
}

static void check_pot() {
	uint8_t d_clamped = read_pot(0, 100);
	uart_send_u16(d_clamped), uart_sendstring("         "), uart_tx('\r');
}

int main(void) {
	uart_init();
	adc_init(ADC_AVCC, ADC_PRESCALER_64);

	while (true) {
		check_pot();
		_delay_ms(50);
	}

	return 0;
}

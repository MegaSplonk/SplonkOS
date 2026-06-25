#include "lib/pinout.h"
#include "lib/proto/adc.h"
#include "lib/proto/rgb.h"
#include "lib/proto/uart.h"
#include <avr/io.h>
#include <util/delay.h>

#define COL1 A5
#define COL2 A4
#define COL3 A3
#define COL4 A2
#define COLS {COL1, COL2, COL3, COL4}

#define ROW1 D4
#define ROW2 D5
#define ROW3 D6
#define ROW4 D7
#define ROWS {ROW1, ROW2, ROW3, ROW4}

const pin		  rows[] = ROWS;
const pin		  cols[] = COLS;
constexpr uint8_t width = tabsize(cols);
constexpr uint8_t height = tabsize(rows);
constexpr uint8_t size = width * height;
rgb				  leds[size] = {};
uint8_t			  brightness[size] = {};
const uint8_t	  led_matrix_idx[] = {0, 1, 2,	3,	4,	5,	6,	7,
									  8, 9, 10, 11, 12, 13, 14, 15};
#define RED ((rgb){255, 0, 0})
#define OFF ((rgb){0, 0, 0})
// void delay_ms(uint16_t ms);

static bool btn_pressed(pin p) {
	bool yes = pin_get(p);
	_delay_ms(5);
	if (yes && pin_get(p)) {
		return true;
	}
	// led_set_color((rgb){-1,0,0}, 1);
	return false;
}

static void check_cols(uint8_t r) {

	for (uint8_t c = 0; c < sizeof(cols) / sizeof(cols[0]); c++) {
		if (btn_pressed(cols[c])) {
			uart_tx('('), uart_send_u8(r), uart_tx(','), uart_send_u8(c),
				uart_tx(')');
			uint8_t idx = led_matrix_idx[c * width + r];
			uart_tx('('), uart_send_u8(idx), uart_tx(')'), uart_tx('\r'),
				uart_tx('\n');
			leds[idx] = (rgb){0, 0, 16};
			brightness[idx] = 15;
		} else {
			// uart_tx((char)(',' + c)), uart_tx('\r');
			// leds[r * height + c] = (rgb){0,0,0};
			// brightness[r * height + c] = 0;
		}
	}
	led_set_colors(leds, brightness, size);
	return;
}

static uint8_t read_pot(uint8_t start, uint8_t end) {
	uint16_t d_pot = adc_read_pin_10bit(ADC_ADC0);
	// uart_send_u16(d_pot), uart_sendstring("         "), uart_tx('\r');
	return (uint8_t)(d_pot * (end - start + 1) / 1024);
}

static void check_pot() {
	uint8_t d_clamped = read_pot(0, size);
	// uart_send_u16(d_clamped), uart_sendstring("         "), uart_tx('\r');
	for (uint8_t i = 0, j = d_clamped > i; i<size; i++, j = d_clamped> i)
		brightness[i] = j ? 15 : 1, leds[i] = j ? RED : OFF;
	led_set_colors(leds, brightness, size);
}

int main(void) {
	uart_init();
	spi_init();
	adc_init(ADC_AVCC, ADC_PRESCALER_64);

	for (uint8_t r = 0; r < sizeof(rows) / sizeof(rows[0]); r++)
		pin_output(rows[r]), pin_set(rows[r], false);
	for (uint8_t c = 0; c < sizeof(cols) / sizeof(cols[0]); c++)
		pin_input(cols[c]);

	for (uint8_t i = 0; i < size; i++)
		brightness[i] = 1;

	led_set_colors(leds, brightness, size); // , tabsize(colors));
	while (true) {
		for (uint8_t r = 0; r < sizeof(rows) / sizeof(rows[0]); r++) {
			pin_set(rows[r], true);
			check_cols(r);
			pin_set(rows[r], false);
		}
		check_pot();
	}

	return 0;
}

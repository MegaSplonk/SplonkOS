#include "lib/config.h"
#include "lib/grb.h"
#include "lib/pinout.h"
#include "lib/proto/uart.h"
#include "lib/time.h"
#include <avr/interrupt.h>
#include <avr/io.h>

#if defined(SPLONKLIB_UART_HARDWARE) && defined(MCU_attiny1614)

#if !defined(BAUD_RATE)
#define BAUD_RATE 115200
#endif

#define UART_TX_PIN P7

GRBIO_INIT(UART_BUF, UART_IBUF, UART_OBUF, UART_IBUF_LEN, UART_OBUF_LEN);

static void uart_ontick_reenable_udre_int(void) {
	TIMER_TICK_500US(1);

	if (!(USART0.CTRLA & USART_DREIE_bm) && !grb_empty(&UART_BUF.output)) {
		uart_enable_udre_int();
	}
}

void uart_poll(void) {
	if ((USART0.STATUS & _BV(USART_DREIF_bm))) {
		u8	  val;
		usize do_tx = 0;
		(void)grb_read(&UART_BUF.output, &val, 1, &do_tx);
		if (do_tx == 1) {
			USART0.TXDATAL = val;
		}
	}
	if (USART0.STATUS & _BV(USART_RXCIF_bm)) {
		u8 val;
		val = USART0.RXDATAL;
		(void)grb_write(&UART_BUF.input, &val, 1, nullptr);
	}
}

void uart_init(void) {
	u8 sreg = SREG;
	SREG &= (u8) ~(_BV(SREG_I));

	pin_output((pin)UART_TX_PIN);

	USART0.BAUD = (u16)(((u32)F_CPU * 64) / (u32)(8 * (u32)BAUD_RATE));
	USART0.CTRLA = USART_RXCIE_bm;
	USART0.CTRLB = USART_RXEN_bm | USART_TXEN_bm | USART_RXMODE_CLK2X_gc;
	USART0.CTRLC = USART_CHSIZE_8BIT_gc;

	timer_ontick_register(T_ONTICK_SLOT_UART, uart_ontick_reenable_udre_int);

	SREG = sreg;
}
void uart_disable_udre_int(void) { USART0.CTRLA &= (u8)~USART_DREIE_bm; }

void uart_enable_udre_int(void) { USART0.CTRLA |= USART_DREIE_bm; }

void uart_tx(u8 data) {
	while (!(USART0.STATUS & USART_DREIF_bm))
		;
	USART0.TXDATAL = data;
}

u8 uart_rx(void) {
	while (!(USART0.STATUS & USART_RXCIF_bm))
		;
	return USART0.RXDATAL;
}

ISR(USART0_RXC_vect) {
	u8	  value;
	usize w;

	value = USART0.RXDATAL;
	(void)grb_write(&UART_BUF.input, &value, 1, &w);
}

ISR(USART0_DRE_vect) {
	u8 value = 'F';

	if (grb_empty(&UART_BUF.output)) {
		uart_disable_udre_int();
		return;
	}

	(void)grb_read(&UART_BUF.output, &value, 1, nullptr);
	USART0.TXDATAL = value;
}

#endif

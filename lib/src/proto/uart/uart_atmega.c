#include "lib/config.h"
#include "lib/grb.h"
#include "lib/pinout.h"
#include "lib/proto/uart.h"
#include "lib/time.h"
#include <avr/interrupt.h>
#include <avr/io.h>

#if defined(SPLONKLIB_UART_HARDWARE) && (defined(MCU_atmega328p) || defined(MCU_atmega32u4))

#if defined(MCU_atmega328p)

#if !defined(BAUD_RATE)
#define BAUD_RATE 115200
#endif

#define UBRR_VALUE ((F_CPU / (8UL * BAUD_RATE)) - 1)

#define UART_RXC	RXC0
#define UART_RXCIE	RXCIE0
#define UART_RXEN	RXEN0
#define UART_TXEN	TXEN0
#define UART_TX_DDR DDRD
#define UART_TX_NUM PD1
#define UART_U2X	U2X0
#define UART_UBRRH	UBRR0H
#define UART_UBRRL	UBRR0L
#define UART_UCSRA	UCSR0A
#define UART_UCSRB	UCSR0B
#define UART_UCSRC	UCSR0C
#define UART_UCSZ0	UCSZ00
#define UART_UCSZ1	UCSZ01
#define UART_UDR	UDR0
#define UART_UDRE	UDRE0
#define UART_UDRIE	UDRIE0

#define UART_UDRE_INTERRUPT USART_UDRE_vect
#define UART_TX_INTERRUPT	USART_TX_vect
#define UART_RX_INTERRUPT	USART_RX_vect

#elif defined(MCU_atmega32u4)

#if !defined(BAUD_RATE)
#define BAUD_RATE 115200
#endif

#define UBRR_VALUE ((F_CPU / (8UL * BAUD_RATE)) - 1)

#define UART_RXC  RXC1
#define UART_RXEN RXEN1
#define UART_TXEN TXEN1

#define UART_U2X U2X1

#define UART_TX_DDR DDRD
#define UART_TX_NUM PD3

#define UART_UBRRH UBRR1H
#define UART_UBRRL UBRR1L

#define UART_UCSRA UCSR1A
#define UART_UCSRB UCSR1B
#define UART_UCSRC UCSR1C

#define UART_UCSZ0 UCSZ10
#define UART_UCSZ1 UCSZ11

#define UART_UDR  UDR1
#define UART_UDRE UDRE1

#define UART_UDRIE UDRIE1
#define UART_RXCIE RXCIE1
#define UART_TXCIE TXCIE1

#define UART_UDRE_INTERRUPT USART1_UDRE_vect
#define UART_TX_INTERRUPT	USART1_TX_vect
#define UART_RX_INTERRUPT	USART1_RX_vect

#endif

GRBIO_INIT(UART_BUF, UART_IBUF, UART_OBUF, UART_IBUF_LEN, UART_OBUF_LEN);

void uart_poll(void) {
	if ((UART_UCSRA & _BV(UART_UDRE))) {
		u8	  val;
		usize do_tx = 0;
		(void)grb_read(&UART_BUF.output, &val, 1, &do_tx);
		if (do_tx == 1) {
			UART_UDR = val;
		}
	}
	if (UART_UCSRA & _BV(UART_RXC)) {
		u8 val;
		val = UART_UDR;
		(void)grb_write(&UART_BUF.input, &val, 1, nullptr);
	}
}

static void uart_ontick_reenable_udre_int(void) {
	TIMER_TICK_500US(1);

	if (!(UART_UCSRB & _BV(UART_UDRIE)) && !grb_empty(&UART_BUF.output)) {
		uart_enable_udre_int();
	}
}

// uart is 115200 baud rate, 8 bits per word, no parity and 1 stop bit
// 115200 8N1
void uart_init(void) {
	// Set baud rate
	UART_UBRRH = (uint8_t)(UBRR_VALUE >> 8);
	UART_UBRRL = (uint8_t)(UBRR_VALUE);

	UART_UCSRA |= _BV(UART_U2X);
	// Enable transmitter
	UART_UCSRB = _BV(UART_TXEN) | _BV(UART_RXEN) |
				 // enable interrupts
				 _BV(UART_RXCIE) | _BV(UART_UDRIE);

	// Set frame format: 8 data bits, no parity, 1 stop bit
	UART_UCSRC = _BV(UART_UCSZ1) | _BV(UART_UCSZ0);
	timer_ontick_register(T_ONTICK_SLOT_UART, uart_ontick_reenable_udre_int);
}

void uart_disable_udre_int(void) {
	// uart_tx('.');
	UART_UCSRB &= (u8)~_BV(UART_UDRIE);
}
void uart_enable_udre_int(void) {

	// uart_tx('@');
	UART_UCSRB |= _BV(UART_UDRIE);
}

void uart_tx(u8 data) {
	// load data into transmit register
	while (!(UART_UCSRA & _BV(UART_UDRE)))
		;
	UART_UDR = data;
}

u8 uart_rx(void) {
	// FIXME: use UART_BUF's grb to read
	while (!(UART_UCSRA & _BV(UART_RXC)))
		;
	return UART_UDR;
}

ISR(UART_UDRE_INTERRUPT) {
	u8 value = 'F';

	if (grb_empty(&UART_BUF.output)) {
		uart_disable_udre_int();
		return;
	}

	(void)grb_read(&UART_BUF.output, &value, 1, nullptr);
	UART_UDR = value;
}

ISR(UART_RX_INTERRUPT) {
	u8	  value;
	usize w;

	value = UART_UDR;
	(void)grb_write(&UART_BUF.input, &value, 1, &w);
}

#endif

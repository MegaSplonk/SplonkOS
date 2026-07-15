#include "lib/proto/uart.h"
#include "lib/config.h"
#include "lib/grb.h"
#include "lib/time.h"
#include <avr/interrupt.h>

#if defined(SPLONKLIB_ENABLE_UART)

#if defined(MCU_atmega328p)

#define BAUD_RATE  115200
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

#define UART_UDRE_INTERUPT USART_UDRE_vect
#define UART_TX_INTERUPT   USART_TX_vect
#define UART_RX_INTERUPT   USART_RX_vect

#elif defined(MCU_atmega32u4)

#define BAUD_RATE  9600
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

#define UART_UDRE_INTERUPT USART1_UDRE_vect
#define UART_TX_INTERUPT   USART1_TX_vect
#define UART_RX_INTERUPT   USART1_RX_vect

#endif

GRBIO_INIT(UART_BUF, UART_IBUF, UART_OBUF, 128, 128);

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
	if (!(UART_UCSRB & _BV(UART_UDRIE)) && !grb_empty(&UART_BUF.output)) {
		uart_enable_udre_int();
	}
}

// uart is 115200 baud rate, 8 bits per word, no parrity and 1 stop bit
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
	uart_tx('.');
	UART_UCSRB &= (u8)~_BV(UART_UDRIE);
}
void uart_enable_udre_int(void) {
	uart_tx('@');
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

void uart_sendstring(const char *str) {
	usize len = 0;
	if (!str)
		return;
	while (str[len])
		len++;
	(void)grb_write(&UART_BUF.output, (const void *)str, len, nullptr);
}
void uart_send_u8(uint8_t val) {
	if (val == 0) {
		uart_tx('0');
		return;
	}
	char	buf[4] = {0, 0, 0, 0};
	uint8_t idx = 0;
	bool	print = false;

	uint8_t modulus = 100;
	while (modulus) {
		uint8_t digit = val / modulus;
		if (print || digit != 0) {
			print = true;
			buf[idx++] = (char)('0' + digit);
		}
		val %= modulus;
		modulus /= 10;
	}

	uart_sendstring(buf);
}

void uart_send_u16(uint16_t val) {
	if (val == 0) {
		uart_tx('0');
		return;
	}
	char	buf[6] = {0, 0, 0, 0, 0, 0};
	uint8_t idx = 0;
	bool	print = false;

	uint16_t modulus = 10000;
	while (modulus) {
		uint8_t digit = (uint8_t)(val / modulus);
		if (print || digit != 0) {
			print = true;
			buf[idx++] = (char)('0' + digit);
		}
		val %= modulus;
		modulus /= 10;
	}
	uart_sendstring(buf);
}

void uart_send_i16(int16_t val) {
	if (val == 0) {
		uart_tx('0');
		return;
	}
	if (val == -32768) {
		uart_sendstring("-32768");
		return;
	}
	if (val < 0) {
		uart_tx('-');
		val = -val;
	}
	uart_send_u16((uint16_t)val);
}

void uart_send_u32(uint32_t val) {
	if (val == 0) {
		uart_tx('0');
		return;
	}
	char	buf[11] = {};
	uint8_t idx = 0;
	bool	print = false;

	uint32_t modulus = 1000000000;
	while (modulus) {
		uint8_t digit = (uint8_t)(val / modulus);
		if (print || digit != 0) {
			print = true;
			buf[idx++] = (char)('0' + digit);
		}
		val %= modulus;
		modulus /= 10;
	}
	uart_sendstring(buf);
}

void uart_send_u8_hex(uint8_t val) {
	char buf[3] = {0, 0, 0};
	buf[0] = "0123456789abcdef"[(val >> 4) & 0x0F];
	buf[1] = "0123456789abcdef"[(val >> 0) & 0x0F];

	uart_sendstring(buf);
}

void uart_send_u16_hex(uint16_t val) {
	char buf[5] = {0, 0, 0, 0, 0};

	buf[0] = "0123456789abcdef"[(val >> 12) & 0x0F];
	buf[1] = "0123456789abcdef"[(val >> 8) & 0x0F];
	buf[2] = "0123456789abcdef"[(val >> 4) & 0x0F];
	buf[3] = "0123456789abcdef"[(val >> 0) & 0x0F];
	uart_sendstring(buf);
}

// one day i'll make these work :D
// the hope is that they will just work in the background, and you just
// read/write to the uart's grb meaning that the data "magically"
// appears/disapears
ISR(UART_UDRE_INTERUPT) {
	u8 value = 'F';
	// usize r = 0;

	if (grb_empty(&UART_BUF.output)) {
		uart_disable_udre_int();
		return;
	}

	(void)grb_read(&UART_BUF.output, &value, 1, nullptr);
	UART_UDR = value;
}

ISR(UART_RX_INTERUPT) {
	u8	  value;
	usize w;

	value = UART_UDR;
	(void)grb_write(&UART_BUF.input, &value, 1, &w);
}

#endif

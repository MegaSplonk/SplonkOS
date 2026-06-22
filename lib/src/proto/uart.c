#include "lib/proto/uart.h"
#include "lib/config.h"

#ifdef SPLONKLIB_ENABLE_UART
#define BAUD_RATE  115200

#define UBRR_VALUE ((F_CPU / (8UL * BAUD_RATE)) - 1)

// uart is 115200 baud rate, 8 bits per word, no parrity and 1 stop bit
// 115200 8N1
void uart_init(void) {
	// Set baud rate
	UBRR0H	= (uint8_t)(UBRR_VALUE >> 8);
	UBRR0L	= (uint8_t)(UBRR_VALUE);

	UCSR0A |= _BV(U2X0);
	// Enable transmitter
	UCSR0B	= _BV(TXEN0) | _BV(RXEN0);

	// Set frame format: 8 data bits, no parity, 1 stop bit
	UCSR0C	= _BV(UCSZ01) | _BV(UCSZ00);

	// Set TX (PD1) as output
	DDRD   |= _BV(PD1);
}

void uart_tx(char data) {
	// wait for transmit buffer to be empty
	while (!(UCSR0A & _BV(UDRE0)))
		;
	// load data into transmit register
	UDR0 = (unsigned char)data;
}

char uart_rx(void) {
	while (!(UCSR0A & _BV(RXC0)))
		;
	return (char)UDR0;
}

void uart_sendstring(const char* str) {
	if (!str)
		return;
	while (*str) {
		uart_tx(*str);
		str++;
	}
}
void uart_send_u8(uint8_t val) {
	if (val == 0){
    uart_tx('0');
		return;
  }
	char	buf[4]	= {0, 0, 0, 0};
	uint8_t idx		= 0;
	bool	print	= false;

	uint8_t modulus = 100;
	while (modulus) {
		uint8_t digit = val / modulus;
		if (print || digit != 0) {
			print	   = true;
			buf[idx++] = (char)('0' + digit);
		}
		val		%= modulus;
		modulus /= 10;
	}

	uart_sendstring(buf);
}

void uart_send_u16(uint16_t val) {
	if (val == 0){
    uart_tx('0');
		return;
  }
	char	buf[6]	 = {0, 0, 0, 0, 0, 0};
	uint8_t idx		 = 0;
	bool	print	 = false;

	uint16_t modulus = 10000;
	while (modulus) {
		uint8_t digit = (uint8_t)(val / modulus);
		if (print || digit != 0) {
			print	   = true;
			buf[idx++] = (char)('0' + digit);
		}
		val		%= modulus;
		modulus /= 10;
	}
	uart_sendstring(buf);
}

void uart_send_i16(int16_t val) {
	if (val == 0){
    uart_tx('0');
		return;
  }
	if (val == -32768){
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
	if (val == 0){
    uart_tx('0');
		return;
  }
	char	buf[11]	 = {};
	uint8_t idx		 = 0;
	bool	print	 = false;

	uint32_t modulus = 1000000000;
	while (modulus) {
		uint8_t digit = (uint8_t)(val / modulus);
		if (print || digit != 0) {
			print	   = true;
			buf[idx++] = (char)('0' + digit);
		}
		val		%= modulus;
		modulus /= 10;
	}
	uart_sendstring(buf);
}

void uart_send_u8_hex(uint8_t val) {
	char buf[3] = {0, 0, 0};
	buf[0]		= "0123456789abcdef"[(val >> 4) & 0x0F];
	buf[1]		= "0123456789abcdef"[(val >> 0) & 0x0F];

	uart_sendstring(buf);
}

void uart_send_u16_hex(uint16_t val) {
	char buf[5] = {0, 0, 0, 0, 0};

	buf[0]		= "0123456789abcdef"[(val >> 12) & 0x0F];
	buf[1]		= "0123456789abcdef"[(val >> 8) & 0x0F];
	buf[2]		= "0123456789abcdef"[(val >> 4) & 0x0F];
	buf[3]		= "0123456789abcdef"[(val >> 0) & 0x0F];
	uart_sendstring(buf);
}
#endif

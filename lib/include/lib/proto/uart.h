#ifndef UART_H
#define UART_H

#include <avr/io.h>
#include <stdint.h>

typedef unsigned char uint8_t;
typedef signed char	  int8_t;

void uart_init(void);

void uart_tx(char data);
void uart_sendstring(const char* str);

void uart_send_u8(uint8_t val);
void uart_send_u16(uint16_t val);
void uart_send_u32(uint32_t val);


void uart_send_i16(int16_t val);

void uart_send_u8_hex(uint8_t val);
void uart_send_u16_hex(uint16_t val);


static inline void print_hex_value(unsigned char c)
{
	uart_send_u8_hex(c);
}

char uart_rx(void);

#endif /* UART_H */


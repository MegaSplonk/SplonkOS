#pragma once
#include <avr/io.h>
#include <stdint.h>

#include "lib/config.h"

#ifdef SPLONKLIB_ENABLE_UART

void uart_init(void);

void uart_tx(u8 data);
void uart_sendstring(const char *str);

void uart_send_u8(uint8_t val);
void uart_send_u16(uint16_t val);
void uart_send_u32(uint32_t val);

void uart_send_i16(int16_t val);

void uart_send_u8_hex(uint8_t val);
void uart_send_u16_hex(uint16_t val);
void uart_poll(void);

static inline void print_hex_value(uint8_t c) { uart_send_u8_hex(c); }

u8 uart_rx(void);

#endif

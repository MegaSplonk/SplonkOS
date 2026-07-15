#pragma once
#include <stdint.h>

typedef volatile uint8_t  *u8_reg;
typedef volatile uint16_t *u16_reg;

typedef uint8_t	  u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef uintptr_t usize;

typedef int8_t	 i8;
typedef int16_t	 i16;
typedef int32_t	 i32;
typedef int64_t	 i64;
typedef intptr_t isize;

#define tabsize(TAB) (sizeof(TAB) / sizeof(TAB[0]))
#define _P(PORT, NB) {.port = PORT_##PORT, .pin = PIN_##NB, ._pad = 0}

#define LOWORD(x) ((uint8_t)((uint16_t)x & (uint16_t)0x00FF))
#define HIWORD(x) ((uint8_t)(((uint16_t)x & (uint16_t)0xFF00) >> 8))

typedef enum : u8 {
	SUCCESS,
	ERROR,
} t_error;

#include <stdio.h>

extern char FMT_BUF[256];

#define debug_print(...)                                                                           \
	do {                                                                                           \
		int16_t n =                                                                                \
			snprintf(FMT_BUF, sizeof(FMT_BUF), "[%s:%i] %s: ", __FILE__, __LINE__, __func__);      \
                                                                                                   \
		if (n < 0 || (uint16_t)n >= sizeof(FMT_BUF)) {                                             \
			uart_sendstring("FAILED TO FORMAT\r\n");                                               \
			break;                                                                                 \
		}                                                                                          \
                                                                                                   \
		snprintf(&FMT_BUF[n], sizeof(FMT_BUF) - (uint16_t)n, __VA_ARGS__);                         \
		uart_sendstring(FMT_BUF);                                                                  \
		uart_sendstring("\r\n");                                                                   \
	} while (0)

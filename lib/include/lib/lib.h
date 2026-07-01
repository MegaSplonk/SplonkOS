#pragma once
#include <avr/io.h>
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

typedef enum : u8 {
	SUCCESS,
	ERROR,
} t_error;

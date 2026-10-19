#pragma once
#include <stdint.h>
extern volatile uint8_t TWCR, TWDR, TWAR, TWSR, TWBR;
#define _BV(bit) (1u << (bit))
#define TWINT	 7
#define TWEA	 6
#define TWSTA	 5
#define TWSTO	 4
#define TWEN	 2
#define TWIE	 0

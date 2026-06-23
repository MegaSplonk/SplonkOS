#pragma once

// YOU SHALL NEVER USE THIS HEADER FILE DIRECTLY !
#include "lib/lib.h"
#include "lib/pin.h"

#define D13 _P(B, 5)
#define A0	_P(C, 0)
#define A1	_P(C, 1)
#define A2	_P(C, 2)
#define A3	_P(C, 3)
#define A4	_P(C, 4)
#define A5	_P(C, 5)

#define D14 A0
#define D15 A1
#define D16 A2
#define D17 A3
#define D18 A4
#define D19 A5

#define D12 _P(B, 4)
#define D11 _P(B, 3)
#define D10 _P(B, 2)
#define D9	_P(B, 1)
#define D8	_P(B, 0)

#define D7 _P(D, 7)
#define D6 _P(D, 6)
#define D5 _P(D, 5)
#define D4 _P(D, 4)
#define D3 _P(D, 3)
#define D2 _P(D, 2)

// TX
#define D1 _P(D, 1)
// RX
#define D0 _P(D, 0)

#define LED_BUILTIN _P(B, 5)
// SPI
#define MISO D12
#define MOSI D11
#define SCLK D13
#define SS	 D10

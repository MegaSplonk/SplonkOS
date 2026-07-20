#pragma once

// YOU SHALL NEVER USE THIS HEADER FILE DIRECTLY !
#include "lib/lib.h"
#include "lib/pin.h"

#define P2	_P(A, 4)
#define P3	_P(A, 5)
#define P4	_P(A, 6)
#define P5	_P(A, 7)
#define P6	_P(B, 3)
#define P7	_P(B, 2)
#define P8	_P(B, 1)
#define P9	_P(B, 0)
#define P10 _P(A, 0)
#define P11 _P(A, 1)
#define P12 _P(A, 2)
#define P13 _P(A, 3)

#define MISO P12
#define MOSI P11
#define SCLK P13
#define SS	 P2

#define SCL P9
#define SDA P8

#define TxD P7
#define RxD P6

#pragma once

// YOU SHALL NEVER USE THIS HEADER FILE DIRECTLY !
#include "lib/lib.h"
#include "lib/pin.h"

// TX
#define D0 (_P(D, 3))
// RX
#define D1 (_P(D, 2))
#define D2 (_P(D, 1))
#define D3 (_P(D, 0))
#define D4 (_P(D, 4))
#define D5 (_P(D, 6))
#define D6 (_P(D, 7))
#define D7 (_P(E, 6))
#define D8 (_P(D, 4))
#define D9 (_P(B, 5))

#define D15 (_P(B, 1))
#define D14 (_P(B, 3))
#define D16 (_P(B, 2))
#define D10 (_P(B, 6))

// RX_LED
#define D17 (_P(B, 0, ))
// TX_LED
#define D24 (_P(D, 5, ))

#define A3 (_P(F, 4))
#define A2 (_P(F, 3))
#define A1 (_P(F, 2))
#define A0 (_P(F, 1))

#define MOSI D16
#define MISO D14
#define SCLK D15

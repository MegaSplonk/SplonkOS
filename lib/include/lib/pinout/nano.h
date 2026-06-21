#pragma once

// YOU SHALL NEVER USE THIS HEADER FILE DIRECTLY !
#include "lib/pin.h"

#define PIN(port, nb) ((pin){.port = PORT_##port, .pin = PIN_##nb, ._pad = 0})

#define D13 PIN(B,5)
#define A0  PIN(C,0)
#define A1  PIN(C,1)
#define A2  PIN(C,2)
#define A3  PIN(C,3)
#define A4  PIN(C,4)
#define A5  PIN(C,5)

#define D14 A0
#define D15 A1
#define D16 A2
#define D17 A3
#define D18 A4
#define D19 A5

#define D12 PIN(B,4)
#define D11 PIN(B,3)
#define D10 PIN(B,2)
#define D9 PIN(B,1)
#define D8 PIN(B,0)

#define D7 PIN(D,7)
#define D6 PIN(D,6)
#define D5 PIN(D,5)
#define D4 PIN(D,4)
#define D3 PIN(D,3)
#define D2 PIN(D,2)

// TX
#define D1 PIN(D,1)
// RX
#define D0 PIN(D,0)

#define LED_BUILTIN PIN(B, 5)

#undef PIN

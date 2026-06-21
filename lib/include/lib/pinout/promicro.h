#pragma once

// YOU SHALL NEVER USE THIS HEADER FILE DIRECTLY !
#include "lib/pin.h"

// TX
#define D0 ((pin){.port = PORT_D, .pin = PIN_3, ._pad = 0})
// RX
#define D1 ((pin){.port = PORT_D, .pin = PIN_2, ._pad = 0})
#define D2 ((pin){.port = PORT_D, .pin = PIN_1, ._pad = 0})
#define D3 ((pin){.port = PORT_D, .pin = PIN_0, ._pad = 0})
#define D4 ((pin){.port = PORT_D, .pin = PIN_4, ._pad = 0})
#define D5 ((pin){.port = PORT_D, .pin = PIN_6, ._pad = 0})
#define D6 ((pin){.port = PORT_D, .pin = PIN_7, ._pad = 0})
#define D7 ((pin){.port = PORT_E, .pin = PIN_6, ._pad = 0})
#define D8 ((pin){.port = PORT_D, .pin = PIN_4, ._pad = 0})
#define D9 ((pin){.port = PORT_B, .pin = PIN_5, ._pad = 0})

#define D15 ((pin){.port = PORT_B, .pin = PIN_1, ._pad = 0})
#define D14 ((pin){.port = PORT_B, .pin = PIN_3, ._pad = 0})
#define D16 ((pin){.port = PORT_B, .pin = PIN_2, ._pad = 0})
#define D10 ((pin){.port = PORT_B, .pin = PIN_6, ._pad = 0})

// RX_LED
#define D17 ((pin){.port = PORT_B, .pin = PIN_0, ._pad = 0})
// TX_LED
#define D24 ((pin){.port = PORT_D, .pin = PIN_5, ._pad = 0})

#define A3 ((pin){.port = PORT_F, .pin = PIN_4, ._pad = 0})
#define A2 ((pin){.port = PORT_F, .pin = PIN_3, ._pad = 0})
#define A1 ((pin){.port = PORT_F, .pin = PIN_2, ._pad = 0})
#define A0 ((pin){.port = PORT_F, .pin = PIN_1, ._pad = 0})



#define MOSI D16
#define MISO D14
#define SCLK D15

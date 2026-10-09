#pragma once

// YOU SHALL NEVER USE THIS HEADER FILE DIRECTLY !
#include "lib/pinout/promicro.h"

#define BUS_ENABLE1 _P(C, 6)
#define BUS_ENABLE2 _P(D, 3)
#define BUS_ENABLE3 _P(D, 2)
#define BUS_ENABLE4 _P(E, 2)

#define ACT_LED _P(C, 7)

#define ROW1 _P(F, 5)
#define ROW2 _P(F, 4)
#define ROW3 _P(F, 1)
#define ROW4 _P(F, 0)

#define KM_ROWS {ROW1, ROW2, ROW3, ROW4}

#define COL1 _P(D, 4)
#define COL2 _P(D, 6)
#define COL3 _P(D, 7)
#define COL4 _P(B, 4)
#define COL5 _P(B, 5)
#define COL6 _P(B, 6)

#define KM_COLS {COL1, COL2, COL3, COL4, COL5, COL6}


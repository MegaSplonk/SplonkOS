#pragma once

// YOU SHALL NEVER USE THIS HEADER FILE DIRECTLY !
#include "lib/pinout/attiny.h"

#define ROW1 _P(A, 2)
#define ROW2 _P(A, 4)

#define KM_ROWS {ROW1, ROW2}

#define COL1 _P(A, 5)
#define COL2 _P(A, 6)
#define COL3 _P(A, 7)

#define KM_COLS {COL1, COL2, COL3}

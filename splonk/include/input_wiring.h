#pragma once
#include "input.h"

// Hardware policy, separate from the decoder and discovery. Current km_scan
// senses rows and probes columns by default. Both splonk halves run the same
// firmware. No mirror transform is assumed before hardware verification.
#ifdef KM_SENSE_COLS
#define SPLONK_WIRE_ORDER INPUT_COLUMNS_FIRST
#else
#define SPLONK_WIRE_ORDER INPUT_ROWS_FIRST
#endif
#ifndef INPUT_SPLONK_L_WIRING
#define INPUT_SPLONK_L_WIRING SPLONK_WIRE_ORDER
#endif
#ifndef INPUT_SPLONK_R_WIRING
#define INPUT_SPLONK_R_WIRING SPLONK_WIRE_ORDER
#endif
#ifndef INPUT_SPLONKO_L_WIRING
#define INPUT_SPLONKO_L_WIRING INPUT_ROWS_FIRST
#endif
#ifndef INPUT_SPLONKO_R_WIRING
#define INPUT_SPLONKO_R_WIRING INPUT_ROWS_FIRST
#endif
#ifndef INPUT_SPLONKUS_WIRING
#define INPUT_SPLONKUS_WIRING INPUT_ROWS_FIRST
#endif

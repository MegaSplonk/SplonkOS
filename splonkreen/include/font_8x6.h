#pragma once

#include "splonkreen.h"

#define FONT_WIDTH		6
#define FONT_HEIGHT		8
#define FONT_FIRST_CHAR 32
#define FONT_LAST_CHAR	126


extern const uint8_t font_8x6[FONT_LAST_CHAR - FONT_FIRST_CHAR + 1][FONT_WIDTH] PROGMEM;
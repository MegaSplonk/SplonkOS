#pragma once
#define PROGMEM
#define pgm_read_word(p) (*(const unsigned short *)(p))

#pragma once

#include "lib/lib.h"
#include <avr/pgmspace.h>

typedef struct global_keymap {
  union {
    u16 all[90];
    struct {
      u16 splonk_l[4][6];
      u16 splonk_r[4][6];
      u16 splonko_l[2][3];
      u16 splonko_r[2][3];
      u16 splonkus[4][6];
      u16 splonknob[6];
    };
  };
} global_keymap;

#ifndef LAYERS
#define LAYERS 3
#endif

#define ROW(...) {__VA_ARGS__}
#define ROW_N(N, ...) [N] = {__VA_ARGS__}

extern const global_keymap  keymaps[LAYERS] PROGMEM;



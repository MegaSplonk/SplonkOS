#pragma once

#ifdef BOARD_promicro
#include "lib/pinout/promicro.h"
#elifdef BOARD_nano
#include "lib/pinout/nano.h"
#else
#error "Board not defined or not known by splonklib"
#endif

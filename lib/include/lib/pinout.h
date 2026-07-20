#pragma once

#if defined(BOARD_promicro)
#include "lib/pinout/promicro.h"
#elif defined(BOARD_nano)
#include "lib/pinout/nano.h"
#elif defined(BOARD_attiny)
#include "lib/pinout/attiny.h"
#else
#error "Board not defined or not known by splonklib"
#endif

#pragma once

#if defined(PINOUT_promicro)
#include "lib/pinout/promicro.h"
#elif defined(PINOUT_nano)
#include "lib/pinout/nano.h"
#elif defined(PINOUT_attiny)
#include "lib/pinout/attiny.h"
#elif defined(PINOUT_splonk)
#include "lib/pinout/splonk.h"
#elif defined(PINOUT_splonko)
#include "lib/pinout/splonko.h"
#elif defined(PINOUT_splonknob)
#include "lib/pinout/splonknob.h"
#else
#error "PINOUT not defined or not known by splonklib"
#endif

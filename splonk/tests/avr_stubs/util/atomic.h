#pragma once
#define ATOMIC_RESTORESTATE 0
#define ATOMIC_BLOCK(mode) for (int atomic_once = 1; atomic_once; atomic_once = 0)

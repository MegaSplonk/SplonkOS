#pragma once

#include "lib/config.h"
#include "lib/pinout.h"

#if defined(KM_ROWS) && defined(KM_COLS)

#define KEY_MATRIX

#define DEBOUNCE 5

constexpr pin	km_rows[] = KM_ROWS;
constexpr usize km_rows_size = tabsize(km_rows);

constexpr pin	km_cols[] = KM_COLS;
constexpr usize km_cols_size = tabsize(km_cols);
constexpr usize km_size = km_rows_size * km_cols_size;

#ifdef KM_SENSE_COLS
constexpr pin km_sense[] = km_cols;
constexpr usize km_sense_size = km_cols_size;
constexpr pin km_probe[] = km_rows;
constexpr usize km_probe_size = km_rows_size;
#else
constexpr pin km_sense[] = km_rows;
constexpr usize km_sense_size = km_rows_size;
constexpr pin km_probe[] = km_cols;
constexpr usize km_probe_size = km_cols_size;
#endif

void km_init(void);
void km_scan(void);

#endif

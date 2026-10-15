#pragma once

#include "lib/config.h"
#include "lib/pinout.h"

#if !defined(RM_ROWS_SIZE) && defined(KM_ROWS)
#define RM_ROWS_SIZE tabsize((pin[])KM_ROWS)
#endif

#if !defined(RM_COLS_SIZE) && defined(KM_COLS)
#define RM_COLS_SIZE tabsize((pin[])KM_COLS)
#endif

#if !defined(RM_SIZE) && defined(RM_ROWS_SIZE) && defined(RM_COLS_SIZE)
#define RM_SIZE (RM_ROWS_SIZE * RM_COLS_SIZE)
#endif

#if defined(RM_SIZE) && defined(SPLONKLIB_ENABLE_LED)

#define RGB_MATRIX

constexpr usize rm_rows_size = RM_ROWS_SIZE;
constexpr usize rm_cols_size = RM_COLS_SIZE;
constexpr usize rm_size = RM_SIZE;

typedef enum led_mode : u8 {
	RM_OFF,
	RM_ON,
	RM_COLOR,

	RM_RAINBOW,
	RM_GD_HOR,
	RM_GD_VER,
	RM_GD_DIA,
	RM_GD_RADIAL,

	RM_SNAKE,
	RM_BREATHING,
	RM_WAVE,
	RM_CHASE,
	RM_FIRE,
	RM_RAIN,

	RM_TOUCH,
	RM_REACTIVE,
	RM_RIPPLE,

	RM_INDICATOR,

	RM_MODE_COUNT

} led_mode;

constexpr led_mode default_led_mode = RM_TOUCH;
constexpr usize	   led_tick_rate = 100;

void rm_init(void);
void rm_set_mode(led_mode m);
void rm_set_keys(u8 *k);
void rm_get_mode(led_mode *m);
void rm_update(void);

#endif

#pragma once

#include "keycodes.h"
#include "lib/lib.h"
#include <avr/pgmspace.h>
#include <stddef.h>

enum keymap_layer {
	LAYER_BASE,
	LAYER_NAV,
	LAYER_NUM,
	LAYERS,
};

// Encode a momentary layer action; execution belongs to the layer resolver.
#define MO(layer)	  (QK_MOMENTARY | (layer))
#define ROW(...)	  {__VA_ARGS__}
#define ROW_N(N, ...) [N] = {__VA_ARGS__}

// Visual row order. The input decoder must translate wiring order into this order.
// Knob entries are l1, r1, b1, l2, r2, b2 (not the wire bitfield order).
typedef struct global_keymap {
	union {
		struct {
			u16 splonk_l[4][6];
			u16 splonk_r[4][6];
			u16 splonko_l[2][3];
			u16 splonko_r[2][3];
			u16 splonkus[4][6];
			u16 splonknob[6];
		};
		u16 all[2 * 4 * 6 + 2 * 2 * 3 + 4 * 6 + 6];
	};
} global_keymap;

// Logical positions, independent of dynamic I2C addresses and USB attachment.
#define KEYMAP_OFFSET(member) (offsetof(global_keymap, member) / sizeof(u16))
enum keymap_offset {
	KEYMAP_SPLONK_L = KEYMAP_OFFSET(splonk_l),
	KEYMAP_SPLONK_R = KEYMAP_OFFSET(splonk_r),
	KEYMAP_SPLONKO_L = KEYMAP_OFFSET(splonko_l),
	KEYMAP_SPLONKO_R = KEYMAP_OFFSET(splonko_r),
	KEYMAP_SPLONKUS = KEYMAP_OFFSET(splonkus),
	KEYMAP_SPLONKNOB = KEYMAP_OFFSET(splonknob),
	KEYMAP_KEY_COUNT = sizeof(global_keymap) / sizeof(u16),
};

static_assert(sizeof(global_keymap) == sizeof(((global_keymap *)0)->all),
			  "keymap fields must match the flat array");
static_assert(LAYERS <= QK_MOMENTARY_MAX - QK_MOMENTARY + 1,
			  "layer IDs must fit momentary keycodes");

extern const global_keymap keymaps[LAYERS] PROGMEM;

// Raw flash lookup: preserves KC_TRNS for the later layer resolver.
// Invalid layer/position returns KC_NO.
u16 keymap_get(u8 layer, u8 position);

#include "keymap.h"

// Higher layers inherit explicitly; zero-filled entries would disable keys.
#define TRNS6 ROW(KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS)
#define TRNS3 ROW(KC_TRNS, KC_TRNS, KC_TRNS)

// Adapted from the Lily58 layout; hold NAV and NUM together for RGB controls.
const global_keymap keymaps[LAYERS] PROGMEM = {
	[LAYER_BASE] = {
		.splonk_l = {
			ROW(RM_TOGG, RM_NEXT, RM_PREV, RM_OFF, RM_ON, KC_TRNS),
			//ROW(KC_ESC, KC_1, KC_2, KC_3, KC_4, KC_5),
			ROW(KC_TAB, KC_Q, KC_W, KC_E, KC_R, KC_T),
			ROW(KC_LSFT, KC_A, KC_S, KC_D, KC_F, KC_G),
			ROW(KC_LCTL, KC_Z, KC_X, KC_C, KC_V, KC_B),
		},
		.splonk_r = {
			ROW(RM_TOGG, RM_NEXT, RM_PREV, RM_OFF, RM_ON, KC_TRNS),
			// ROW(KC_6, KC_7, KC_8, KC_9, KC_0, KC_MINS),
			ROW(KC_Y, KC_U, KC_I, KC_O, KC_P, KC_BSPC),
			ROW(KC_H, KC_J, KC_K, KC_L, KC_SCLN, KC_QUOT),
			ROW(KC_N, KC_M, KC_COMM, KC_DOT, KC_SLSH, KC_RSFT),
		},
		.splonko_l = {ROW(KC_LGUI, KC_LALT, MO(LAYER_NAV)), ROW(KC_SPC, KC_SPC, KC_LBRC)},
		.splonko_r = {ROW(KC_ENT, KC_ENT, KC_RBRC), ROW(KC_RGUI, KC_RALT, MO(LAYER_NUM))},
		// Standalone numpad plus navigation; unused positions are intentionally off.
		.splonkus = {
			ROW(KC_NUM, KC_PSLS, KC_PAST, KC_PMNS, KC_HOME, KC_PGUP),
			ROW(KC_P7, KC_P8, KC_P9, KC_PPLS, KC_END, KC_PGDN),
			ROW(KC_P4, KC_P5, KC_P6, KC_PENT, KC_NO, KC_NO),
			ROW(KC_P1, KC_P2, KC_P3, KC_P0, KC_PDOT, KC_NO),
		},
		.splonknob = ROW(KC_AUDIO_VOL_DOWN, KC_AUDIO_VOL_UP, KC_AUDIO_MUTE,
						 KC_MEDIA_PREV_TRACK, KC_MEDIA_NEXT_TRACK, KC_MEDIA_PLAY_PAUSE),
	},
	[LAYER_NAV] = {
		.splonk_l = {
			ROW(KC_F1, KC_F2, KC_F3, KC_F4, KC_F5, KC_F6),
			ROW(KC_F7, KC_F8, KC_F9, KC_F10, KC_F11, KC_F12),
			TRNS6,
			TRNS6,
		},
		.splonk_r = {
			ROW(KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_EQL),
			ROW(KC_PGUP, KC_UP, KC_PGDN, KC_TRNS, KC_TRNS, KC_BSLS),
			ROW(KC_LEFT, KC_DOWN, KC_RGHT, KC_TRNS, KC_TRNS, KC_TRNS),
			ROW(KC_DEL, KC_HOME, KC_END, KC_TRNS, KC_TRNS, KC_TRNS),
		},
		.splonko_l = {TRNS3, ROW(KC_TRNS, KC_TRNS, KC_CAPS)},
		.splonko_r = {TRNS3, TRNS3},
		.splonkus = {TRNS6, TRNS6, TRNS6, TRNS6},
		.splonknob = TRNS6,
	},
	[LAYER_NUM] = {
		.splonk_l = {
			ROW(KC_F13, KC_F14, KC_F15, KC_F16, KC_F17, KC_F18),
			ROW(KC_F19, KC_F20, KC_F21, KC_F22, KC_F23, KC_F24),
			TRNS6,
			TRNS6,
		},
		.splonk_r = {
			ROW(KC_NUM, KC_PSLS, KC_PAST, KC_PMNS, KC_TRNS, KC_TRNS),
			ROW(KC_P7, KC_P8, KC_P9, KC_PPLS, KC_TRNS, KC_TRNS),
			ROW(KC_P4, KC_P5, KC_P6, KC_PCMM, KC_TRNS, KC_TRNS),
			ROW(KC_P1, KC_P2, KC_P3, KC_PEQL, KC_TRNS, KC_TRNS),
		},
		.splonko_l = {TRNS3, TRNS3},
		// Keep MO(NUM) reachable through transparency; Enter remains available.
		.splonko_r = {ROW(KC_PENT, KC_PENT, KC_TRNS), ROW(KC_P0, KC_PDOT, KC_TRNS)},
		.splonkus = {TRNS6, TRNS6, TRNS6, TRNS6},
		.splonknob = TRNS6,
	},
	[LAYER_RGB] = {
		.splonk_l = {
			ROW(RM_TOGG, RM_NEXT, RM_PREV, RM_OFF, RM_ON, KC_TRNS),
			TRNS6, TRNS6, TRNS6,
		},
		.splonk_r = {
			ROW(RM_PREV, RM_NEXT, RM_TOGG, RM_OFF, RM_ON, KC_TRNS),
			TRNS6, TRNS6, TRNS6,
		},
		.splonko_l = {ROW(RM_PREV, RM_NEXT, RM_TOGG), ROW(RM_OFF, RM_ON, KC_TRNS)},
		.splonko_r = {ROW(RM_TOGG, RM_NEXT, RM_PREV), ROW(RM_ON, RM_OFF, KC_TRNS)},
		.splonkus = {TRNS6, TRNS6, TRNS6, TRNS6},
		.splonknob = TRNS6,
	},
};

u16 keymap_get(u8 layer, u8 position) {
	if (layer >= LAYERS || position >= KEYMAP_KEY_COUNT)
		return KC_NO;
	return pgm_read_word(&keymaps[layer].all[position]);
}

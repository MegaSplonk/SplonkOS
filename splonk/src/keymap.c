#include "keymap.h"
#include "keycodes.h"
#include <avr/pgmspace.h>

/*
Original Lily58 layout define:

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
	[0] = LAYOUT(QK_GESC, KC_1, KC_2, KC_3, KC_4, KC_5, KC_6, KC_7, KC_8, KC_9, KC_0   , KC_MINS,
KC_TAB, KC_Q, KC_W, KC_E, KC_R, KC_T, KC_Y, KC_U, KC_I, KC_O, KC_P   , KC_BSPC, KC_LSFT, KC_A, KC_S,
KC_D, KC_F, KC_G, KC_H, KC_J, KC_K, KC_L, KC_SCLN, KC_QUOT, KC_LCTL, KC_Z, KC_X, KC_C, KC_V, KC_B,
KC_LBRC, KC_RBRC, KC_N, KC_M, KC_COMM, KC_DOT, KC_SLSH, KC_RSFT, KC_LGUI, KC_LALT, MO(1), KC_SPC,
KC_ENT, MO(2), KC_RALT, KC_RGUI), [1] = LAYOUT(KC_F1, KC_F2, KC_F3, KC_F4, KC_F5, KC_F6, _______,
_______, _______, _______, _______, KC_EQL, KC_F7, KC_F8, KC_F9, KC_F10, KC_F11, KC_F12, KC_PGUP,
KC_UP, KC_PGDN, _______, _______, KC_BSLS, _______, _______, _______, _______, _______, _______,
KC_LEFT, KC_DOWN, KC_RGHT, _______, _______, _______, _______, _______, _______, _______, _______,
_______, _______, _______, KC_DEL, KC_HOME, KC_END, _______, _______, KC_CAPS, _______, _______,
_______, _______, _______, MO(3), _______, _______), [2] = LAYOUT(KC_F13, KC_F14, KC_F15, KC_F16,
KC_F17, KC_F18, KC_NUM, KC_PSLS, KC_PAST, KC_PMNS, _______, _______, KC_F19, KC_F20, KC_F21, KC_F22,
KC_F23, KC_F24, KC_P7, KC_P8, KC_P9, KC_PPLS, _______, _______, _______, _______, _______, _______,
_______, _______, KC_P4, KC_P5, KC_P6, KC_PCMM, _______, _______, _______, _______, _______,
_______, _______, _______, _______, _______, KC_P1, KC_P2, KC_P3, KC_PEQL, _______, _______,
_______, _______, MO(3), _______, _______, KC_P0, KC_PDOT, KC_PENT), [3] = LAYOUT(XXXXXXX, XXXXXXX,
XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,
XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,
XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, BL_STEP, BL_UP, RGB_MOD, RGB_M_R, RGB_SPI, RGB_HUI, RGB_SAI,
RGB_VAI, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, BL_BRTG, BL_DOWN, BL_TOGG, RGB_TOG, RGB_RMOD, RGB_M_T,
RGB_SPD, RGB_HUD, RGB_SAD, RGB_VAD, _______, _______, _______, _______, _______, _______, _______,
_______)
};

adapted into our format (to be continued):
*/

const global_keymap PROGMEM
	keymaps[LAYERS] =
		{
			[0] =
				{
					.splonk_l = {ROW(QK_GESC, KC_1, KC_2, KC_3, KC_4, KC_5),
								 ROW(KC_TAB, KC_Q, KC_W, KC_E, KC_R, KC_T),
								 ROW(KC_LSFT, KC_A, KC_S, KC_D, KC_F, KC_G),
								 ROW(KC_LCTL, KC_Z, KC_X, KC_C, KC_V, KC_B)},
					.splonk_r = {ROW(KC_6, KC_7, KC_8, KC_9, KC_0, KC_MINS),
								 ROW(KC_Y, KC_U, KC_I, KC_O, KC_P, KC_BSPC),
								 ROW(KC_H, KC_J, KC_K, KC_L, KC_SCLN, KC_QUOT),
								 ROW(KC_N, KC_M, KC_COMM, KC_DOT, KC_SLSH, KC_RSFT)},
					.splonko_l = {ROW(KC_LGUI, KC_LALT, MO(1)), ROW(KC_SPC, KC_SPC, KC_LBRC)},
					.splonko_r = {ROW(KC_ENT, KC_ENT, KC_RBRC), ROW(KC_RGUI, KC_RALT, MO(2))},
					.splonkus = {/* define a 6x4 numpad? macropad? depending on layer? */},
					.splonknob =
						{ROW(KC_AUDIO_VOL_DOWN, KC_AUDIO_VOL_UP, KC_AUDIO_MUTE, KC_MEDIA_PREV_TRACK,
							 KC_MEDIA_NEXT_TRACK, KC_MEDIA_PLAY_PAUSE)},
				},
			[1] =
				{
					.splonk_l = {ROW(KC_F1, KC_F2, KC_F3, KC_F4, KC_F5, KC_F6),
								 ROW(KC_F7, KC_F8, KC_F9, KC_F10, KC_F11, KC_F12),
								 ROW(_______, _______, _______, _______, _______, _______),
								 ROW(_______, _______, _______, _______, _______, _______)},
					.splonk_r = {ROW(_______, _______, _______, _______, _______, KC_EQL),
								 ROW(KC_PGUP, KC_UP, KC_PGDN, _______, _______, KC_BSLS),
								 ROW(KC_LEFT, KC_DOWN, KC_RGHT, _______, _______, _______),
								 ROW(KC_N, KC_M, KC_COMM, KC_DOT, KC_SLSH, KC_RSFT)},
					.splonko_l = {ROW(KC_LGUI, KC_LALT, MO(1)), ROW(KC_SPC, KC_SPC, KC_LBRC)},
					.splonko_r = {ROW(KC_ENT, KC_ENT, KC_RBRC), ROW(KC_RGUI, KC_RALT, MO(2))},
				},
			// To be continued
};

/*
since the Lily58 is one keyboard whereas our splonk and splonko are different hardware parts
communicating through I2C, I deemed a struct to be a cleaner approach...
The code that translates the i2c data into usb hid reports by using the following keymaps needs to
be adapted accordingly.
*/

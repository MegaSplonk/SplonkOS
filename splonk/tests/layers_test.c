#define keymap_get flash_keymap_get
#include "../src/keymap.c"
#undef keymap_get
#include "layers.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static bool fixture;
u16			keymap_get(u8 layer, u8 position) {
	if (fixture) {
		if (position == 0 || position == 1)
			return MO(LAYER_NAV);
		if (position == 2)
			return MO(31); // Invalid layer must not escape to HID.
		if (position == 3 && layer == LAYER_NAV)
			return KC_NO;
	}
	return flash_keymap_get(layer, position);
}
static layer_state state;
static struct {
	u16	 code;
	bool pressed;
} actions[64];
static unsigned count;
static void		emit(u16 code, bool pressed, void *context) {
	(void)context;
	assert(count < 64);
	actions[count].code = code;
	actions[count++].pressed = pressed;
}
static void button(input_module module, u8 position, bool pressed) {
	input_event event = {.source = (u8)module,
						 .module = module,
						 .kind = INPUT_BUTTON,
						 .button = {position, pressed}};
	layers_input(&state, &event, emit, nullptr);
}
int main(void) {
	button(INPUT_SPLONK_L, 1, true);  // base 1
	button(INPUT_SPLONKO_L, 2, true); // NAV
	button(INPUT_SPLONK_L, 2, true);  // F3
	button(INPUT_SPLONKO_R, 5, true); // NAV + NUM activates RGB
	button(INPUT_SPLONK_L, 3, true);  // RGB off
	button(INPUT_SPLONK_L, 13, true); // transparent A
	assert(actions[0].code == KC_1 && actions[1].code == KC_F3);
	assert(actions[2].code == RM_OFF && actions[3].code == KC_A);
	button(INPUT_SPLONKO_L, 2, false);
	button(INPUT_SPLONKO_R, 5, false);
	button(INPUT_SPLONK_L, 2, false);
	assert(actions[4].code == KC_F3 && !actions[4].pressed);
	button(INPUT_SPLONK_L, 1, false);
	assert(actions[5].code == KC_1 && !actions[5].pressed);
	button(INPUT_SPLONK_L, 3, false); // Release retains RGB action after layers change.
	assert(actions[6].code == RM_OFF && !actions[6].pressed);
	unsigned before = count;
	button(INPUT_SPLONK_L, 1, false); // duplicate release
	button(INPUT_SPLONK_L, 24, true); // invalid position
	assert(count == before);
	layers_reset(&state, emit, nullptr);
	assert(!state.holds[LAYER_NAV] && !state.holds[LAYER_NUM]);
	// Exercise both activation orders and leaving RGB by releasing either hold.
	for (u8 order = 0; order < 2; order++) {
		input_module first = order ? INPUT_SPLONKO_R : INPUT_SPLONKO_L;
		input_module second = order ? INPUT_SPLONKO_L : INPUT_SPLONKO_R;
		u8			 first_pos = order ? 5 : 2;
		u8			 second_pos = order ? 2 : 5;
		u8			 resolved;
		button(first, first_pos, true);
		assert(layers_lookup(&state, KEYMAP_SPLONK_L, &resolved) == (order ? KC_F13 : KC_F1));
		assert(resolved == (order ? LAYER_NUM : LAYER_NAV));
		button(second, second_pos, true);
		assert(layers_lookup(&state, KEYMAP_SPLONK_L, &resolved) == RM_TOGG);
		assert(resolved == LAYER_RGB);
		// Transparent RGB positions still resolve through NUM before NAV.
		assert(layers_lookup(&state, KEYMAP_SPLONK_L + 5, &resolved) == KC_F18);
		assert(resolved == LAYER_NUM);
		button(first, first_pos, false);
		assert(layers_lookup(&state, KEYMAP_SPLONK_L, &resolved) == (order ? KC_F1 : KC_F13));
		assert(resolved == (order ? LAYER_NAV : LAYER_NUM));
		button(second, second_pos, false);
		assert(layers_lookup(&state, KEYMAP_SPLONK_L, &resolved) == KC_ESC);
		assert(resolved == LAYER_BASE);
		assert(!state.holds[LAYER_NAV] && !state.holds[LAYER_NUM]);
	}
	fixture = true;
	count = 0;
	button(INPUT_SPLONK_L, 0, true);
	button(INPUT_SPLONK_L, 1, true);
	assert(state.holds[LAYER_NAV] == 2);
	button(INPUT_SPLONK_L, 0, false);
	assert(state.holds[LAYER_NAV] == 1);
	button(INPUT_SPLONK_L, 2, true);
	button(INPUT_SPLONK_L, 3, true); // NO blocks lower layer
	assert(count == 0);
	input_event reset = {.source = INPUT_SPLONK_L, .module = INPUT_SPLONK_L, .kind = INPUT_RESET};
	layers_input(&state, &reset, emit, nullptr);
	assert(!state.holds[LAYER_NAV]);
	fixture = false;
	input_event step = {.source = 8, .module = INPUT_SPLONKNOB, .kind = INPUT_STEP, .step = 1};
	layers_input(&state, &step, emit, nullptr);
	layers_input(&state, &step, emit, nullptr);
	assert(count == 4 && actions[0].code == KC_AUDIO_VOL_UP);
	assert(actions[0].pressed && !actions[1].pressed && actions[2].pressed && !actions[3].pressed);
	puts("layer resolution and ownership tests passed");
}

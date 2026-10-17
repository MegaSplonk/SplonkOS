#define keymap_get flash_keymap_get
#include "../src/keymap.c"
#undef keymap_get
#include "layers.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static bool fixture;
u16 keymap_get(u8 layer, u8 position) {
	if (fixture) {
		if (position == 0 || position == 1) return MO(LAYER_NAV);
		if (position == 2) return MO(31); // Invalid layer must not escape to HID.
		if (position == 3 && layer == LAYER_NAV) return KC_NO;
	}
	return flash_keymap_get(layer, position);
}
static layer_state state;
static struct { u16 code; bool pressed; } actions[64];
static unsigned count;
static void emit(u16 code, bool pressed, void *context) {
	(void)context;
	assert(count < 64);
	actions[count].code = code;
	actions[count++].pressed = pressed;
}
static void button(input_module module, u8 position, bool pressed) {
	input_event event = {.source = (u8)module, .module = module, .kind = INPUT_BUTTON,
		.button = {position, pressed}};
	layers_input(&state, &event, emit, nullptr);
}
int main(void) {
	button(INPUT_SPLONK_L, 1, true); // base 1
	button(INPUT_SPLONKO_L, 2, true); // NAV
	button(INPUT_SPLONK_L, 2, true); // F3
	button(INPUT_SPLONKO_R, 5, true); // NUM wins over NAV
	button(INPUT_SPLONK_L, 3, true); // F16
	button(INPUT_SPLONK_L, 13, true); // transparent A
	assert(actions[0].code == KC_1 && actions[1].code == KC_F3);
	assert(actions[2].code == KC_F16 && actions[3].code == KC_A);
	button(INPUT_SPLONKO_L, 2, false);
	button(INPUT_SPLONKO_R, 5, false);
	button(INPUT_SPLONK_L, 2, false);
	assert(actions[4].code == KC_F3 && !actions[4].pressed);
	button(INPUT_SPLONK_L, 1, false);
	assert(actions[5].code == KC_1 && !actions[5].pressed);
	unsigned before = count;
	button(INPUT_SPLONK_L, 1, false); // duplicate release
	button(INPUT_SPLONK_L, 24, true); // invalid position
	assert(count == before);
	layers_reset(&state, emit, nullptr);
	assert(!state.holds[LAYER_NAV] && !state.holds[LAYER_NUM]);
	fixture = true; count = 0;
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

#include "layers.h"
#include <string.h>

static bool module_range(input_module module, u8 *offset, u8 *count) {
	switch (module) {
	case INPUT_SPLONK_L:
		*offset = KEYMAP_SPLONK_L;
		*count = 24;
		break;
	case INPUT_SPLONK_R:
		*offset = KEYMAP_SPLONK_R;
		*count = 24;
		break;
	case INPUT_SPLONKO_L:
		*offset = KEYMAP_SPLONKO_L;
		*count = 6;
		break;
	case INPUT_SPLONKO_R:
		*offset = KEYMAP_SPLONKO_R;
		*count = 6;
		break;
	case INPUT_SPLONKUS:
		*offset = KEYMAP_SPLONKUS;
		*count = 24;
		break;
	case INPUT_SPLONKNOB:
		*offset = KEYMAP_SPLONKNOB;
		*count = 6;
		break;
	default:
		return false;
	}
	return true;
}

u16 layers_lookup(const layer_state *state, u8 position, u8 *resolved_layer) {
	for (u8 i = LAYERS; i > 0; i--) {
		u8 layer = i - 1;
		if (layer && !state->holds[layer])
			continue;
		u16 code = keymap_get(layer, position);
		if (code == KC_TRNS)
			continue;
		if (resolved_layer)
			*resolved_layer = layer;
		return code;
	}
	if (resolved_layer)
		*resolved_layer = LAYER_BASE;
	return KC_NO;
}

static void action(layer_state *state, u16 code, bool pressed, action_emit emit, void *context) {
	if (code >= QK_MOMENTARY && code <= QK_MOMENTARY_MAX) {
		u16 layer = code - QK_MOMENTARY;
		if (layer > LAYER_BASE && layer < LAYERS) {
			if (pressed)
				state->holds[layer]++;
			else if (state->holds[layer])
				state->holds[layer]--;
		}
	} else if (code != KC_NO && code != KC_TRNS && emit)
		emit(code, pressed, context);
}

static void release(layer_state *state, u8 position, action_emit emit, void *context) {
	if (!state->held[position])
		return;
	u16 code = keymap_get(state->held[position] - 1, position);
	state->held[position] = 0;
	action(state, code, false, emit, context);
}

void layers_input(layer_state *state, const input_event *event, action_emit emit, void *context) {
	u8 offset, count;
	if (!module_range(event->module, &offset, &count) || event->source == 255)
		return;
	u8 *owner = &state->owner[event->module];
	if (*owner && *owner != event->source + 1)
		return;
	if (event->kind == INPUT_RESET) {
		for (u8 i = 0; i < count; i++)
			release(state, offset + i, emit, context);
		*owner = 0;
		return;
	}
	if (event->kind != INPUT_BUTTON && event->kind != INPUT_STEP)
		return;
	u8 position = event->kind == INPUT_STEP ? event->step : event->button.position;
	if (position >= count)
		return;
	*owner = event->source + 1;
	position += offset;
	if (event->kind == INPUT_BUTTON && !event->button.pressed) {
		release(state, position, emit, context);
		return;
	}
	if (state->held[position])
		return;
	u8	layer;
	u16 code = layers_lookup(state, position, &layer);
	if (event->kind == INPUT_BUTTON)
		state->held[position] = layer + 1;
	action(state, code, true, emit, context);
	if (event->kind == INPUT_STEP)
		action(state, code, false, emit, context);
}

void layers_reset(layer_state *state, action_emit emit, void *context) {
	for (u8 i = 0; i < KEYMAP_KEY_COUNT; i++)
		release(state, i, emit, context);
	memset(state, 0, sizeof(*state));
}

u16 layers_held_code(const layer_state *state, u8 position) {
	if (position >= KEYMAP_KEY_COUNT || !state->held[position])
		return KC_NO;
	return keymap_get(state->held[position] - 1, position);
}
bool layers_holds_code(const layer_state *state, u16 code) {
	for (u8 i = 0; i < KEYMAP_KEY_COUNT; i++)
		if (layers_held_code(state, i) == code)
			return true;
	return false;
}

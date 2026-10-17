#pragma once
#include "input.h"
#include "keymap.h"

typedef void (*action_emit)(u16 code, bool pressed, void *context);
typedef struct layer_state {
	// Immutable flash keymap: retaining the resolved layer retains the action,
	// using one byte per position instead of two. Zero means not held.
	u8 held[KEYMAP_KEY_COUNT];
	u8 owner[INPUT_MODULE_COUNT]; // source + 1, zero = unbound
	u8 holds[LAYERS];
} layer_state;

// Zero-initialize. Events are processed in delivery order: matrix releases
// precede presses, then ascending physical position. A layer affects subsequent
// events, never retroactively remaps already held keys.
void layers_input(layer_state *state, const input_event *event, action_emit emit, void *context);
void layers_reset(layer_state *state, action_emit emit, void *context);
u16	 layers_lookup(const layer_state *state, u8 position, u8 *resolved_layer);

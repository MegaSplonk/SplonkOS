#include "input.h"

static u8 rows(input_module module) {
	switch (module) {
	case INPUT_SPLONK_L:
	case INPUT_SPLONK_R:
	case INPUT_SPLONKUS:
		return 4;
	case INPUT_SPLONKO_L:
	case INPUT_SPLONKO_R:
		return 2;
	default:
		return 0;
	}
}

static void transition(input_device *device, u8 source, const u8 next[3], input_emit emit,
					   void *ctx) {
	// Releases precede presses in each snapshot. Layers are resolved later.
	for (u8 pass = 0; pass < 2; pass++) {
		for (u8 key = 0; key < 24; key++) {
			u8	 mask = (u8)(1u << (key % 8));
			bool before = (device->held[key / 8] & mask) != 0;
			bool after = (next[key / 8] & mask) != 0;
			if (before != after && after == (pass != 0)) {
				input_event e = {.source = source,
								 .module = device->module,
								 .kind = INPUT_BUTTON,
								 .button = {key, after}};
				emit(&e, ctx);
			}
		}
	}
	for (u8 i = 0; i < 3; i++)
		device->held[i] = next[i];
}

bool input_bind(input_device *device, u8 source, input_module module, u8 wiring, input_emit emit,
				void *ctx) {
	if (!device || !emit || module >= INPUT_MODULE_COUNT || wiring > 7 ||
		(!rows(module) && wiring != 0))
		return false;
	if (device->module == module && device->wiring == wiring)
		return true;
	if (rows(device->module) || device->module == INPUT_SPLONKNOB) {
		const u8 released[3] = {};
		transition(device, source, released, emit, ctx);
	} else if (device->module == INPUT_SPLONKPAD && device->buttons) {
		(void)input_pointer(device, source, 0, 0, 0, emit, ctx);
	}
	if (device->module != INPUT_NONE) {
		input_event e = {.source = source, .module = device->module, .kind = INPUT_RESET};
		emit(&e, ctx);
	}
	*device = (input_device){.module = module, .wiring = wiring};
	return true;
}

bool input_feed(input_device *device, u8 source, const u8 *bytes, usize length, input_emit emit,
				void *ctx) {
	if (!device || !bytes || !emit)
		return false;
	u8	 row_count = rows(device->module);
	bool knob = device->module == INPUT_SPLONKNOB;
	if ((!row_count && !knob) || length != (row_count == 4 ? 3u : 1u))
		return false;
	u8 next[3] = {};
	if (knob) {
		if (bytes[0] & (1u << 1))
			next[0] |= 1u << 2;
		if (bytes[0] & (1u << 4))
			next[0] |= 1u << 5;
	} else {
		u8 columns = row_count == 4 ? 6 : 3;
		for (u8 bit = 0; bit < row_count * columns; bit++) {
			if (!(bytes[bit / 8] & (1u << (bit % 8))))
				continue;
			u8 row = device->wiring & INPUT_COLUMNS_FIRST ? bit % row_count : bit / columns;
			u8 col = device->wiring & INPUT_COLUMNS_FIRST ? bit / row_count : bit % columns;
			if (device->wiring & INPUT_MIRROR_COLUMNS)
				col = (u8)(columns - 1 - col);
			if (device->wiring & INPUT_MIRROR_ROWS)
				row = (u8)(row_count - 1 - row);
			u8 key = (u8)(row * columns + col);
			next[key / 8] |= (u8)(1u << (key % 8));
		}
	}
	transition(device, source, next, emit, ctx);
	if (knob) {
		for (u8 bit = 0; bit < 6; bit++) {
			if (bit == 1 || bit == 4 || !(bytes[0] & (1u << bit)))
				continue;
			input_event e = {.source = source,
							 .module = device->module,
							 .kind = INPUT_STEP,
							 .step = bit == 2	? 1
									 : bit == 5 ? 4
												: bit};
			emit(&e, ctx);
		}
	}
	return true;
}

bool input_absolute(input_device *device, u8 source, u16 value, input_emit emit, void *ctx) {
	if (!device || !emit || device->module != INPUT_SPLONKBAR)
		return false;
	if (device->axis.valid && device->axis.value == value)
		return true;
	device->axis.value = value;
	device->axis.valid = true;
	input_event e = {
		.source = source, .module = device->module, .kind = INPUT_ABSOLUTE, .absolute = value};
	emit(&e, ctx);
	return true;
}

bool input_pointer(input_device *device, u8 source, i16 dx, i16 dy, u8 buttons, input_emit emit,
				   void *ctx) {
	if (!device || !emit || device->module != INPUT_SPLONKPAD)
		return false;
	if (!dx && !dy && device->buttons == buttons)
		return true;
	device->buttons = buttons;
	input_event e = {.source = source,
					 .module = device->module,
					 .kind = INPUT_POINTER,
					 .pointer = {dx, dy, buttons}};
	emit(&e, ctx);
	return true;
}

bool input_tap(input_device *device, u8 source, u8 count, input_emit emit, void *ctx) {
	if (!device || !emit || device->module != INPUT_SPLONKPAD || count < 1 || count > 3)
		return false;
	input_event e = {.source = source, .module = device->module, .kind = INPUT_TAP, .taps = count};
	emit(&e, ctx);
	return true;
}

bool input_source_bind(input_source *source, u8 slot, u8 identity, input_module module, u8 wiring,
					   input_emit emit, void *context) {
	if (!source || !emit || module >= INPUT_MODULE_COUNT || wiring > 7 ||
		(!rows(module) && wiring != 0) || (!identity && module != INPUT_NONE))
		return false;
	if (source->identity != identity) {
		(void)input_bind(&source->decoder, slot, INPUT_NONE, 0, emit, context);
		source->identity = identity;
		source->length = 0;
	}
	bool changed = source->decoder.module != module || source->decoder.wiring != wiring;
	(void)input_bind(&source->decoder, slot, module, wiring, emit, context);
	if (changed && source->length && module != INPUT_NONE)
		(void)input_feed(&source->decoder, slot, source->cached, source->length, emit, context);
	return true;
}

bool input_source_feed(input_source *source, u8 slot, const u8 *bytes, u8 length, bool knob,
					   input_emit emit, void *context) {
	if (!source || !source->identity || !bytes || !emit || (length != 1 && length != 3) ||
		(knob && length != 1))
		return false;
	if (source->decoder.module != INPUT_NONE) {
		if (knob != (source->decoder.module == INPUT_SPLONKNOB) ||
			!input_feed(&source->decoder, slot, bytes, length, emit, context))
			return false;
	}
	for (u8 i = 0; i < length; i++)
		source->cached[i] = bytes[i];
	source->length = length;
	if (knob)
		source->cached[0] &= 0x12; // Persistent b1/b2 only, even while unbound.
	return true;
}

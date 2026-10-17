#include "input.h"
#include "input_packets.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static input_event events[128];
static usize	   count;
static void		   capture(const input_event *event, void *context) {
	assert(context == events);
	assert(count < tabsize(events));
	events[count++] = *event;
}
static void bind(input_device *d, u8 source, input_module module, u8 wiring) {
	assert(input_bind(d, source, module, wiring, capture, events));
}
static void feed(input_device *d, u8 source, const u8 *bytes, usize length) {
	assert(input_feed(d, source, bytes, length, capture, events));
}
static void button(usize i, u8 source, input_module module, u8 key, bool pressed) {
	assert(events[i].source == source && events[i].module == module);
	assert(events[i].kind == INPUT_BUTTON && events[i].button.position == key);
	assert(events[i].button.pressed == pressed);
}

int main(void) {
	// Every matrix key, across byte boundaries and both halves, independently.
	for (input_module module = INPUT_SPLONK_L; module <= INPUT_SPLONKUS; module++) {
		input_device d = {};
		bind(&d, 2, module, 0);
		u8 n = module == INPUT_SPLONKO_L || module == INPUT_SPLONKO_R ? 6 : 24;
		for (u8 key = 0; key < n; key++) {
			u8 bytes[3] = {};
			bytes[key / 8] = (u8)(1u << (key % 8));
			count = 0;
			feed(&d, 2, bytes, n == 6 ? 1 : 3);
			assert(count == 1);
			button(0, 2, module, key, true);
			feed(&d, 2, bytes, n == 6 ? 1 : 3);
			assert(count == 1); // Duplicate levels do not retrigger.
			memset(bytes, 0, sizeof(bytes));
			feed(&d, 2, bytes, n == 6 ? 1 : 3);
			assert(count == 2);
			button(1, 2, module, key, false);
		}
	}
	input_device left = {}, right = {};
	bind(&left, 1, INPUT_SPLONK_L, 0);
	bind(&right, 2, INPUT_SPLONK_R, 0);
	const u8 a[3] = {1}, b[3] = {2}, zero[3] = {};
	count = 0;
	feed(&left, 1, a, 3);
	feed(&right, 2, a, 3);
	feed(&left, 1, zero, 3);
	assert(count == 3 && right.held[0] == 1);
	button(2, 1, INPUT_SPLONK_L, 0, false);
	// Truncated/oversized/null packets must not release held keys.
	assert(!input_feed(&right, 2, zero, 2, capture, events));
	assert(!input_feed(&right, 2, zero, 4, capture, events));
	assert(!input_feed(&right, 2, nullptr, 3, capture, events));
	assert(right.held[0] == 1 && count == 3);
	count = 0;
	feed(&right, 2, b, 3);
	assert(count == 2);
	button(0, 2, INPUT_SPLONK_R, 0, false);
	button(1, 2, INPUT_SPLONK_R, 1, true);
	bind(&right, 2, INPUT_SPLONKO_R, 0);
	button(2, 2, INPUT_SPLONK_R, 1, false);
	assert(events[3].kind == INPUT_RESET && events[3].module == INPUT_SPLONK_R);
	count = 0;
	const u8 thumb = 0xc0;
	feed(&right, 2, &thumb, 1);
	assert(count == 0); // Padding ignored.
	feed(&right, 2, a, 1);
	bind(&right, 2, INPUT_NONE, 0);
	button(1, 2, INPUT_SPLONKO_R, 0, false);
	assert(events[2].kind == INPUT_RESET);
	assert(!input_feed(&right, 2, a, 1, capture, events));

	// Explicit coordinate expectations for alternate probe order and mirrors.
	const struct {
		u8 flags, bit, expected;
	} mappings[] = {
		{INPUT_COLUMNS_FIRST, 1, 6},
		{INPUT_COLUMNS_FIRST, 4, 1},
		{INPUT_MIRROR_COLUMNS, 0, 5},
		{INPUT_MIRROR_ROWS, 0, 18},
		{INPUT_MIRROR_COLUMNS | INPUT_MIRROR_ROWS, 0, 23},
		{INPUT_COLUMNS_FIRST | INPUT_MIRROR_COLUMNS, 4, 4},
	};
	for (usize i = 0; i < tabsize(mappings); i++) {
		input_device d = {};
		bind(&d, 1, INPUT_SPLONK_L, mappings[i].flags);
		u8 bytes[3] = {};
		bytes[mappings[i].bit / 8] = (u8)(1u << (mappings[i].bit % 8));
		count = 0;
		feed(&d, 1, bytes, 3);
		assert(count == 1);
		button(0, 1, INPUT_SPLONK_L, mappings[i].expected, true);
	}

	input_device knob = {};
	bind(&knob, 3, INPUT_SPLONKNOB, 0);
	const u8 step = 0x01, all = 0xff, buttons = 0x12;
	count = 0;
	feed(&knob, 3, &step, 1);
	feed(&knob, 3, &step, 1);
	assert(count == 2 && events[0].kind == INPUT_STEP && events[1].step == 0);
	count = 0;
	feed(&knob, 3, &all, 1);
	assert(count == 6);
	button(0, 3, INPUT_SPLONKNOB, 2, true);
	button(1, 3, INPUT_SPLONKNOB, 5, true);
	assert(events[2].step == 0 && events[3].step == 1 && events[4].step == 3 &&
		   events[5].step == 4);
	count = 0;
	feed(&knob, 3, &buttons, 1);
	assert(count == 0);
	bind(&knob, 3, INPUT_NONE, 0);
	assert(count == 3);
	button(0, 3, INPUT_SPLONKNOB, 2, false);
	button(1, 3, INPUT_SPLONKNOB, 5, false);

	input_device bar = {}, pad = {};
	bind(&bar, 4, INPUT_SPLONKBAR, 0);
	bind(&pad, 5, INPUT_SPLONKPAD, 0);
	count = 0;
	assert(input_absolute(&bar, 4, 0, capture, events)); // Initial zero is still a sample.
	assert(input_absolute(&bar, 4, 0, capture, events));
	assert(input_absolute(&bar, 4, 65535, capture, events));
	assert(count == 2 && events[1].absolute == 65535);
	assert(!input_absolute(&pad, 5, 1, capture, events));
	bind(&bar, 4, INPUT_NONE, 0);
	assert(events[2].kind == INPUT_RESET);
	bind(&bar, 4, INPUT_SPLONKBAR, 0);
	assert(input_absolute(&bar, 4, 65535, capture, events));
	assert(count == 4);
	count = 0;
	assert(input_pointer(&pad, 5, -300, 400, 1, capture, events));
	assert(input_pointer(&pad, 5, -300, 400, 1, capture, events)); // Relative motion repeats.
	assert(input_pointer(&pad, 5, 0, 0, 1, capture, events));
	assert(count == 2);
	assert(events[0].pointer.dx == -300 && events[0].pointer.dy == 400);
	for (u8 taps = 1; taps <= 3; taps++)
		assert(input_tap(&pad, 5, taps, capture, events));
	assert(count == 5 && events[4].taps == 3);
	assert(!input_tap(&pad, 5, 0, capture, events));
	assert(!input_tap(&pad, 5, 4, capture, events));
	bind(&pad, 5, INPUT_NONE, 0);
	assert(events[5].kind == INPUT_POINTER && events[5].pointer.buttons == 0);
	assert(events[6].kind == INPUT_RESET);

	// Real wire sizes and real ring-buffer framing: no partial consumption.
	assert(sizeof(splonko) == 1 && sizeof(splonkus) == 3 && sizeof(rgb_control) == 1);
	assert(input_packet_size(IPT_SPLONKO) == 1 && input_packet_size(IPT_SPLONKUS) == 3);
	assert(input_packet_size(IPT_SPLONKBAR) == 0 && input_packet_size(IPT_SPLONKPAD) == 0);
	u8	storage[6];
	grb buffer = GRB_INIT(storage);
	u8	packet[3];
	grb_write(&buffer, a, 2, nullptr);
	assert(!input_packet_read(&buffer, packet, 3) && grb_count(&buffer) == 2);
	grb_write(&buffer, a + 2, 1, nullptr);
	grb_write(&buffer, zero, 3, nullptr);
	input_device queue = {};
	bind(&queue, 6, INPUT_SPLONKUS, 0);
	count = 0;
	assert(input_packet_read(&buffer, packet, 3));
	feed(&queue, 6, packet, 3);
	assert(input_packet_read(&buffer, packet, 3));
	feed(&queue, 6, packet, 3);
	assert(count == 2);
	button(0, 6, INPUT_SPLONKUS, 0, true);
	button(1, 6, INPUT_SPLONKUS, 0, false);
	assert(!input_packet_read(&buffer, packet, 0) && !input_packet_read(&buffer, nullptr, 3));
	// A change-only sender can arrive before topology is known. Restore only
	// current levels on role/ownership changes, and never replay old steps.
	input_source source = {};
	count = 0;
	assert(input_source_bind(&source, 7, 60, INPUT_NONE, 0, capture, events));
	assert(input_source_feed(&source, 7, a, 1, false, capture, events));
	assert(count == 0);
	assert(input_source_bind(&source, 7, 60, INPUT_SPLONKO_R, 0, capture, events));
	assert(count == 1);
	button(0, 7, INPUT_SPLONKO_R, 0, true);
	assert(input_source_bind(&source, 7, 60, INPUT_NONE, 0, capture, events));
	assert(count == 3);
	button(1, 7, INPUT_SPLONKO_R, 0, false);
	assert(input_source_bind(&source, 7, 60, INPUT_SPLONKO_R, 0, capture, events));
	assert(count == 4);
	button(3, 7, INPUT_SPLONKO_R, 0, true);
	assert(input_source_bind(&source, 7, 61, INPUT_SPLONKO_R, 0, capture, events));
	assert(count == 6 && source.length == 0); // New identity cannot inherit old press.
	assert(input_source_bind(&source, 7, 0, INPUT_NONE, 0, capture, events));
	assert(!input_source_feed(&source, 7, a, 1, false, capture, events));
	assert(input_source_bind(&source, 7, 62, INPUT_NONE, 0, capture, events));
	count = 0;
	assert(input_source_feed(&source, 7, &all, 1, true, capture, events));
	assert(count == 0);
	assert(input_source_bind(&source, 7, 62, INPUT_SPLONKNOB, 0, capture, events));
	assert(count == 2); // Two held buttons, no historical rotations.
	assert(!input_source_feed(&source, 7, a, 3, true, capture, events));
	assert(count == 2);
	puts("input decoder and packet tests passed");
}

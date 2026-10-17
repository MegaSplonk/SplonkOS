#include "lib/usb/hid.h"

#include "report.h"
#include <string.h>

#define HID_REPORT(_TYPE, _ID, _NAME)                                                              \
	struct _TYPE _buf_report_##_NAME = {};                                                         \
	hid_report	 report_##_NAME = {.report = &_buf_report_##_NAME,                                 \
								   .id = (_ID),                                                    \
								   .pending = false,                                               \
								   .len = sizeof(_buf_report_##_NAME)}

HID_REPORT(HID_NKRO_report, HID_REPORT_ID_NKRO, nkro);
HID_REPORT(HID_MOUSE_report, HID_REPORT_ID_MOUSE, mouse);
HID_REPORT(HID_CC_report, HID_REPORT_ID_CC, cc);
HID_REPORT(HID_SLIDER_report, HID_REPORT_ID_SLIDER, slider);

hid_report *reports[] = {
	&report_nkro,
	&report_mouse,
	&report_cc,
	&report_slider,
};

const usize reports_size = tabsize(reports);

#define ACTION_CAPACITY 32
static u16				  actions[ACTION_CAPACITY];
static u8				  head, count;
static bool				  active, recovering;
static const layer_state *held_state;
u16						  report_overflows;

bool report_translate(u16 code, key *action) {
	if ((code >= 4 && code <= HID_NKRO_USAGE_MAX) || (code >= 0xE0 && code <= 0xE7)) {
		*action = (key)KK(SCANCODE, (u8)code);
		return true;
	}
	u8 consumer;
	switch (code) {
	case KC_AUDIO_VOL_UP:
		consumer = KK_CC_VOL_UP;
		break;
	case KC_AUDIO_VOL_DOWN:
		consumer = KK_CC_VOL_DOWN;
		break;
	case KC_AUDIO_MUTE:
		consumer = KK_CC_MUTE;
		break;
	case KC_MEDIA_PLAY_PAUSE:
		consumer = KK_CC_PAUSE_PLAY;
		break;
	case KC_MEDIA_NEXT_TRACK:
		consumer = KK_CC_NEXT;
		break;
	case KC_MEDIA_PREV_TRACK:
		consumer = KK_CC_PREVIOUS;
		break;
	default:
		return false;
	}
	*action = (key)KK(CC, consumer);
	return true;
}

void report_reset(bool enabled) {
	active = enabled;
	head = count = 0;
	recovering = false;
	for (usize i = 0; i < reports_size; i++) {
		memset(reports[i]->report, 0, reports[i]->len);
		reports[i]->pending = enabled;
	}
}
void report_init(const layer_state *state) {
	held_state = state;
	report_overflows = 0;
	report_reset(false);
}

void report_action(u16 code, bool pressed, void *context) {
	(void)context;
	key action;
	if (!active || recovering || !report_translate(code, &action))
		return;
	// The resolver has already removed this physical hold. Another physical
	// binding of the same usage must keep it down (e.g. the two Space keys).
	if (!pressed && held_state && layers_holds_code(held_state, code))
		return;
	handle_usb();
	if (count == ACTION_CAPACITY) {
		// Bound memory and latency. Losing transient input under sustained USB
		// backpressure is preferable to retaining an unmatched press forever.
		report_overflows++;
		report_reset(true);
		recovering = true;
		return;
	}
	actions[(head + count) % ACTION_CAPACITY] = code | (pressed ? 0 : 0x8000);
	count++;
	handle_usb();
}

static bool send_pending(void) {
	for (u8 i = 0; i < reports_size; i++) {
		hid_report *report = reports[i];
		if (!report->pending)
			continue;
		if (hid_send_report(report->id, report->report, report->len) != (isize)(report->len + 1))
			return false;
		report->pending = false;
		if (report->id == HID_REPORT_ID_MOUSE) {
			struct HID_MOUSE_report *mouse = report->report;
			mouse->x = mouse->y = mouse->wheel = 0;
		}
	}
	return true;
}

void handle_usb(void) {
	if (!active || !send_pending())
		return;
	if (recovering) {
		recovering = false;
		if (held_state) {
			for (u8 i = 0; i < KEYMAP_KEY_COUNT; i++) {
				key action;
				if (report_translate(layers_held_code(held_state, i), &action))
					kk_apply_key(action);
			}
		}
		if (!send_pending())
			return;
	}
	// Every accepted edge reaches USB before the next edge mutates its report.
	// Stop immediately on backpressure; the pending report remains unchanged.
	for (u8 budget = 0; count && budget < ACTION_CAPACITY; budget++) {
		u16 encoded = actions[head];
		head = (u8)((head + 1u) % ACTION_CAPACITY);
		count--;
		key action;
		if (report_translate(encoded & 0x7FFF, &action))
			(encoded & 0x8000 ? kk_unapply_key : kk_apply_key)(action);
		if (!send_pending())
			return;
	}
}

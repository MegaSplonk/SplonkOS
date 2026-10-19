#include "report.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void			   uart_sendstring(const char *s) { (void)s; }
static layer_state state;
static bool		   blocked;
static unsigned	   sent;
static struct {
	u8	  id, bytes[32];
	usize length;
} frames[512];
isize hid_send_report(enum e_hid_report_id id, const void *data, usize len) {
	if (blocked)
		return -1;
	assert(sent < 512 && len <= 32);
	frames[sent].id = id;
	frames[sent].length = len;
	memcpy(frames[sent++].bytes, data, len);
	return (isize)(len + 1);
}
static void button(input_module module, u8 pos, bool down) {
	input_event e = {
		.source = (u8)module, .module = module, .kind = INPUT_BUTTON, .button = {pos, down}};
	layers_input(&state, &e, report_action, nullptr);
}
static bool has_key(unsigned frame, u8 keycode) {
	assert(frames[frame].id == HID_REPORT_ID_NKRO);
	return (frames[frame].bytes[2 + keycode / 8] & (1u << (keycode % 8))) != 0;
}
static void fresh(void) {
	memset(&state, 0, sizeof(state));
	blocked = false;
	report_init(&state);
	report_reset(true);
	handle_usb();
	sent = 0;
}
int main(void) {
	fresh();
	key k;
	assert(!report_translate(KC_NO, &k) && !report_translate(KC_TRNS, &k));
	assert(!report_translate(MO(1), &k) && !report_translate(KC_SYSTEM_POWER, &k));
	assert(report_translate(KC_AUDIO_VOL_UP, &k) && k.kind == KK_CC && k.payload == KK_CC_VOL_UP);
	// RGB actions are handled before HID; only the five implemented controls are allowed.
	for (u8 layer = 0; layer < LAYERS; layer++)
		for (u8 pos = 0; pos < KEYMAP_KEY_COUNT; pos++) {
			u16 code = keymap_get(layer, pos);
			if (IS_RGB_MATRIX_KEYCODE(code)) {
				assert(code == RM_ON || code == RM_OFF || code == RM_TOGG || code == RM_NEXT ||
					   code == RM_PREV);
				assert(!report_translate(code, &k));
				report_action(code, true, nullptr);
				report_action(code, false, nullptr);
				assert(sent == 0); // RGB must never leak into a USB report.
				continue;
			}
			assert(code == KC_NO || code == KC_TRNS ||
				   (code >= QK_MOMENTARY && code <= QK_MOMENTARY_MAX) ||
				   report_translate(code, &k));
		}
	button(INPUT_SPLONK_L, 12, true); // LSHIFT
	button(INPUT_SPLONK_R, 23, true); // RSHIFT
	assert(frames[0].bytes[0] == 2 && frames[1].bytes[0] == 0x22);
	button(INPUT_SPLONK_L, 12, false);
	assert(frames[2].bytes[0] == 0x20);
	button(INPUT_SPLONK_R, 23, false);
	assert(frames[3].bytes[0] == 0);
	fresh();
	button(INPUT_SPLONKO_L, 3, true); // two Space bindings
	button(INPUT_SPLONKO_L, 4, true);
	unsigned before = sent;
	button(INPUT_SPLONKO_L, 3, false);
	assert(sent == before && has_key(sent - 1, KC_SPACE));
	button(INPUT_SPLONKO_L, 4, false);
	assert(!has_key(sent - 1, KC_SPACE));
	fresh();
	button(INPUT_SPLONKO_R, 5, true); // NUM
	button(INPUT_SPLONK_L, 11, true); // F24
	assert(has_key(sent - 1, KC_F24));
	button(INPUT_SPLONK_R, 15, true); // KP comma
	assert(has_key(sent - 1, KC_KP_COMMA));
	button(INPUT_SPLONK_R, 21, true); // KP equal
	assert(has_key(sent - 1, KC_KP_EQUAL));
	button(INPUT_SPLONKO_R, 5, false);
	button(INPUT_SPLONK_L, 11, false);
	assert(!has_key(sent - 1, KC_F24) && has_key(sent - 1, KC_KP_COMMA));
	fresh();
	blocked = true;
	button(INPUT_SPLONK_L, 13, true); // fast A tap while endpoint blocked
	button(INPUT_SPLONK_L, 13, false);
	handle_usb();
	assert(sent == 0);
	blocked = false;
	handle_usb();
	assert(sent == 2 && has_key(0, KC_A) && !has_key(1, KC_A));
	fresh();
	blocked = true;
	input_event step = {.source = 8, .module = INPUT_SPLONKNOB, .kind = INPUT_STEP, .step = 1};
	for (u8 i = 0; i < 5; i++)
		layers_input(&state, &step, report_action, nullptr);
	blocked = false;
	handle_usb();
	assert(sent == 10);
	for (unsigned i = 0; i < sent; i++) {
		assert(frames[i].id == HID_REPORT_ID_CC && frames[i].length == 1);
		assert(frames[i].bytes[0] == (i % 2 ? 0 : 1));
	}
	fresh();
	blocked = true;
	button(INPUT_SPLONK_L, 13, true);
	for (u8 i = 0; i < 40; i++)
		layers_input(&state, &step, report_action, nullptr);
	assert(report_overflows == 1);
	blocked = false;
	handle_usb();
	assert(frames[0].id == HID_REPORT_ID_NKRO && !has_key(0, KC_A)); // all up first
	assert(has_key(sent - 1, KC_A)); // rebuild held A, no phantom encoder hold
	button(INPUT_SPLONK_L, 13, false);
	assert(!has_key(sent - 1, KC_A));
	fresh();
	blocked = true;
	button(INPUT_SPLONK_L, 13, true);
	report_reset(false); // detach discards queued old-owner actions
	blocked = false;
	handle_usb();
	assert(sent == 0);
	layers_reset(&state, report_action, nullptr);
	report_reset(true);
	handle_usb();
	assert(!has_key(0, KC_A));
	// Low-level relative deltas are consumed only on successful transmission.
	fresh();
	blocked = true;
	kk_apply_key((key)KK(MOUSE, KK_MOUSE_RIGHT));
	handle_usb();
	assert(sent == 0);
	blocked = false;
	handle_usb();
	assert(sent == 1 && frames[0].length == sizeof(struct HID_MOUSE_report));
	assert(frames[0].bytes[1] == KK_MOUSE_SENSITIVITY);
	kk_apply_key((key)KK(MOUSE, KK_MOUSE_LBTN));
	handle_usb();
	assert(frames[1].bytes[0] == 1 && frames[1].bytes[1] == 0);
	// The bar uses the 40-bit absolute slider report and retains the latest
	// sample while USB is blocked.
	fresh();
	blocked = true;
	report_slider_value(341);
	report_slider_value(682);
	assert(sent == 0);
	blocked = false;
	handle_usb();
	assert(sent == 1 && frames[0].id == HID_REPORT_ID_SLIDER);
	assert(frames[0].length == 5);
	assert(((u16)frames[0].bytes[0] | ((u16)(frames[0].bytes[1] & 3) << 8)) == 682);
	assert(frames[0].bytes[2] == 0 && frames[0].bytes[3] == 0 && frames[0].bytes[4] == 0);
	report_slider_value(682);
	assert(sent == 1);
	report_slider_value(1200);
	assert(sent == 2 && frames[1].id == HID_REPORT_ID_SLIDER);
	assert(((u16)frames[1].bytes[0] | ((u16)(frames[1].bytes[1] & 3) << 8)) == 1023);
	puts("HID translation, ownership, retry and overflow tests passed");
}
